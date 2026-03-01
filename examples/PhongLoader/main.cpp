// examples/rotating_obj_windowed_center_fit_phong.cpp
//
// Loads an OBJ (Group-per-material supported), fits camera, rotates around its bounds center,
// and renders with Phong shading.
//
// Usage: rotating_obj_windowed_center_fit_phong.exe path/to/model.obj

#include <memory>
#include <string>
#include <chrono>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Viewer.h"
#include "Group.h"
#include "ObjLoader.h"
#include "Shader.h"
#include "BoundsUtils.h"

int main(int argc, char** argv)
{
    using namespace bonsai;
    using clock_t = std::chrono::steady_clock;

    if (argc < 2)
    {
        std::cout << "Usage: rotating_obj_windowed_center_fit_phong <model.obj>\n";
        return 0;
    }

    const std::string objPath = argv[1];

    Viewer viewer;
    if (!viewer.open(900, 700, "Rotating OBJ (Phong) - Windowed"))
        return 1;

    // --- Phong shader ---
    //
    // Uses built-ins (gl_Vertex, gl_Normal) so it works with your current client arrays.
    // Note: normal transform here assumes uniform scale (good enough for your demo).
    const std::string vs = R"GLSL(
        uniform mat4 uModel;
        uniform mat4 uView;
        uniform mat4 uProj;
        uniform mat4 uMVP;

        varying vec3 vPosW;
        varying vec3 vNrmW;

        void main()
        {
            vec4 posW = uModel * gl_Vertex;
            vPosW = posW.xyz;

            // Approx normal transform (ok for uniform scale)
            vNrmW = normalize((uModel * vec4(gl_Normal, 0.0)).xyz);

            gl_Position = uMVP * gl_Vertex;
        }
    )GLSL";

    const std::string fs = R"GLSL(
        varying vec3 vPosW;
        varying vec3 vNrmW;

        uniform vec3 uEyePosW;     // camera position in world space
        uniform vec3 uLightDirW;   // direction TO light (normalized)
        uniform vec3 uLightColor;  // light color/intensity
        uniform vec3 uBaseColor;   // material base color
        uniform float uAmbient;    // ambient factor
        uniform float uShininess;  // spec power
        uniform float uSpecular;   // spec factor

        void main()
        {
            vec3 N = normalize(vNrmW);
            vec3 L = normalize(uLightDirW);
            vec3 V = normalize(uEyePosW - vPosW);

            // Diffuse
            float NdotL = max(dot(N, L), 0.0);

            // Specular (Blinn-Phong)
            vec3 H = normalize(L + V);
            float NdotH = max(dot(N, H), 0.0);
            float spec = pow(NdotH, uShininess) * uSpecular;

            vec3 color =
                uBaseColor * (uAmbient + NdotL) * uLightColor +
                spec * uLightColor;

            gl_FragColor = vec4(color, 1.0);
        }
    )GLSL";

    auto shader = std::make_shared<Shader>();
    if (!shader->buildFromSource(vs, fs))
        return 2;

    viewer.setShader(shader);

    // Load OBJ -> Group node + bounds (from loader)
    ObjLoader::Options opt;
    opt.loadTexcoords = true;
    opt.loadNormals = true;
    opt.computeNormalsIfMissing = true;
    opt.flipV = false;
    opt.scale = 1.0f;
    opt.groupByMaterial = true;

    bonsai::util::Bounds bounds;
    NodePtr modelRoot = ObjLoader::load(objPath, opt, &bounds);
    if (!modelRoot)
    {
        std::cout << "Failed to load OBJ: " << objPath << "\n";
        return 3;
    }

    // Put model under a parent so we can animate cleanly
    auto root = std::make_shared<Group>();
    root->addChild(modelRoot);
    viewer.setSceneData(root);

    // Fit camera using bounds at FOV 40
    const float fovY = 40.0f;
    bonsai::util::BoundsUtils::fitCameraToBounds(viewer, bounds, fovY);

    // To set uEyePosW we need to know where the camera is.
    // BoundsUtils fits the camera along +Z by default:
    //   eye = center + normalize(viewDir) * dist
    // So we can reconstruct the same eye position here.
    auto computeEyeFromFit = [&](float aspect) -> glm::vec3
    {
        const float fov = glm::radians(fovY);
        const float dist = (bounds.radius / std::tan(fov * 0.5f)) * 1.25f;
        return bounds.center + glm::vec3(0, 0, 1) * dist;
    };

    // Animation
    const float degPerSec = 30.0f;
    const auto t0 = clock_t::now();

    while (viewer.frame())
    {
        const float seconds = std::chrono::duration<float>(clock_t::now() - t0).count();
        const float a = glm::radians(degPerSec * seconds);

        // Rotate around bounds.center
        glm::mat4 M(1.0f);
        M = glm::translate(M, bounds.center);
        M = glm::rotate(M, a, glm::vec3(0, 1, 0));
        M = glm::rotate(M, a * 0.6f, glm::vec3(1, 0, 0));
        M = glm::translate(M, -bounds.center);
        root->setTransform(M);

        // Keep projection + view correct on resize
        bonsai::util::BoundsUtils::fitCameraToBounds(viewer, bounds, fovY);

        // Compute current eye position (matches fitCameraToBounds default viewDir=(0,0,1))
        const float aspect = (viewer.height() > 0) ? (float)viewer.width() / (float)viewer.height() : 1.0f;
        glm::vec3 eye = computeEyeFromFit(aspect);

        // Set Phong uniforms (per-frame is fine for demo)
        shader->use();
        shader->setUniform("uEyePosW", eye);
        shader->setUniform("uLightDirW", glm::normalize(glm::vec3(0.4f, 0.8f, 0.6f)));
        shader->setUniform("uLightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        shader->setUniform("uBaseColor", glm::vec3(0.85f, 0.85f, 0.9f));
        shader->setUniform("uAmbient", 0.15f);
        shader->setUniform("uSpecular", 0.6f);
        shader->setUniform("uShininess", 48.0f);
        Shader::unuse();
    }

    viewer.close();
    return 0;
}