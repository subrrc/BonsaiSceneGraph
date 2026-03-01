// examples/rotating_obj_phong_per_material.cpp
//
// Loads an OBJ, uses Phong shading, assigns different uBaseColor per material group
// based on "mtl_<name>" group names, fits camera to bounds, and rotates around the model center.
//
// Usage: rotating_obj_phong_per_material.exe path/to/model.obj

#include <memory>
#include <string>
#include <chrono>
#include <iostream>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Viewer.h"
#include "Group.h"
#include "Geode.h"
#include "ObjLoader.h"
#include "Shader.h"
#include "BoundsUtils.h"
#include "UniformDrawable.h"
#include "RenderState.h"

// Inserts a UniformDrawable at the front of each "mtl_*" group to set uBaseColor
static void applyMaterialBaseColors(
    const std::shared_ptr<bonsai::Group>& modelRoot,
    const std::unordered_map<std::string, glm::vec3>& colorMap,
    const glm::vec3& defaultColor = glm::vec3(0.85f, 0.85f, 0.9f))
{
    if (!modelRoot) return;

    for (const auto& child : modelRoot->getChildren())
    {
        auto mtlGroup = std::dynamic_pointer_cast<bonsai::Group>(child);
        if (!mtlGroup) continue;

        const std::string groupName = mtlGroup->getName(); // e.g. "mtl_brass"
        const std::string prefix = "mtl_";
        if (groupName.rfind(prefix, 0) != 0)
            continue;

        const std::string mtlName = groupName.substr(prefix.size());

        glm::vec3 color = defaultColor;
        auto it = colorMap.find(mtlName);
        if (it != colorMap.end())
            color = it->second;

        auto setterGeode = std::make_shared<bonsai::Geode>();
        setterGeode->setName("set_uBaseColor_" + mtlName);

        auto setter = std::make_shared<bonsai::UniformDrawable>(
            [color](const bonsai::RenderState& state)
            {
                if (state.shader && state.shader->isValid())
                {
                    state.shader->use();
                    state.shader->setUniform("uBaseColor", color);
                    bonsai::Shader::unuse();
                }
            });

        setterGeode->addDrawable(setter);

        // Must be first so it runs before material group's geometry draws
        mtlGroup->insertChild(0, setterGeode);
    }
}

int main(int argc, char** argv)
{
    using clock_t = std::chrono::steady_clock;

    if (argc < 2)
    {
        std::cout << "Usage: rotating_obj_phong_per_material <model.obj>\n";
        return 0;
    }

    const std::string objPath = argv[1];

    bonsai::Viewer viewer;
    if (!viewer.open(900, 700, "OBJ Phong + Per-Material uBaseColor"))
        return 1;

    // Phong shader (world-space)
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

            // Approx normal transform (good for uniform scale)
            vNrmW = normalize((uModel * vec4(gl_Normal, 0.0)).xyz);

            gl_Position = uMVP * gl_Vertex;
        }
    )GLSL";

    const std::string fs = R"GLSL(
        varying vec3 vPosW;
        varying vec3 vNrmW;

        uniform vec3 uEyePosW;
        uniform vec3 uLightDirW;   // direction TO light
        uniform vec3 uLightColor;
        uniform vec3 uBaseColor;

        uniform float uAmbient;
        uniform float uSpecular;
        uniform float uShininess;

        void main()
        {
            vec3 N = normalize(vNrmW);
            vec3 L = normalize(uLightDirW);
            vec3 V = normalize(uEyePosW - vPosW);

            float NdotL = max(dot(N, L), 0.0);

            // Blinn-Phong
            vec3 H = normalize(L + V);
            float spec = pow(max(dot(N, H), 0.0), uShininess) * uSpecular;

            vec3 color = uBaseColor * (uAmbient + NdotL) * uLightColor
                       + spec * uLightColor;

            gl_FragColor = vec4(color, 1.0);
        }
    )GLSL";

    auto shader = std::make_shared<bonsai::Shader>();
    if (!shader->buildFromSource(vs, fs))
        return 2;

    viewer.setShader(shader);

    // Load OBJ -> Group root per material + bounds
    bonsai::ObjLoader::Options opt;
    opt.loadTexcoords = true;
    opt.loadNormals = true;
    opt.computeNormalsIfMissing = true;
    opt.flipV = false;
    opt.scale = 1.0f;
    opt.groupByMaterial = true;

    bonsai::util::Bounds bounds;
    bonsai::NodePtr modelRoot = bonsai::ObjLoader::load(objPath, opt, &bounds);
    if (!modelRoot)
    {
        std::cout << "Failed to load OBJ: " << objPath << "\n";
        return 3;
    }

    // Assign per-material colors
    auto modelGroup = std::dynamic_pointer_cast<bonsai::Group>(modelRoot);
    if (modelGroup)
    {
        // Example material -> base color mapping:
        std::unordered_map<std::string, glm::vec3> colors = {
            {"default", {0.85f, 0.85f, 0.90f}},
            {"brass",   {0.85f, 0.65f, 0.25f}},
            {"plastic", {0.20f, 0.70f, 0.90f}},
            {"rubber",  {0.10f, 0.10f, 0.12f}},
            {"red",     {0.85f, 0.20f, 0.20f}},
            {"green",   {0.20f, 0.85f, 0.20f}},
            {"blue",    {0.20f, 0.20f, 0.90f}},
        };

        applyMaterialBaseColors(modelGroup, colors);
    }

    // Put model under a parent root so we can animate transform cleanly
    auto root = std::make_shared<bonsai::Group>();
    root->addChild(modelRoot);
    viewer.setSceneData(root);

    // Fit camera at FOV 40
    const float fovY = 40.0f;
    bonsai::util::BoundsUtils::fitCameraToBounds(viewer, bounds, fovY);

    // Reconstruct eye position matching BoundsUtils default viewDir=(0,0,1)
    auto computeEye = [&]() -> glm::vec3
    {
        const float fov = glm::radians(fovY);
        const float dist = (bounds.radius / std::tan(fov * 0.5f)) * 1.25f;
        return bounds.center + glm::vec3(0, 0, 1) * dist;
    };

    // Set constant light/material uniforms (uBaseColor is set per material group)
    {
        glm::vec3 eye = computeEye();
        shader->use();
        shader->setUniform("uEyePosW", eye);
        shader->setUniform("uLightDirW", glm::normalize(glm::vec3(0.4f, 0.8f, 0.6f)));
        shader->setUniform("uLightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        shader->setUniform("uAmbient", 0.15f);
        shader->setUniform("uSpecular", 0.6f);
        shader->setUniform("uShininess", 48.0f);

        // Set a default base color in case a material group doesn't set one
        shader->setUniform("uBaseColor", glm::vec3(0.85f, 0.85f, 0.9f));
        bonsai::Shader::unuse();
    }

    // Rotate around bounds center
    const float degPerSec = 30.0f;
    const auto t0 = clock_t::now();

    while (viewer.frame())
    {
        const float seconds = std::chrono::duration<float>(clock_t::now() - t0).count();
        const float a = glm::radians(degPerSec * seconds);

        glm::mat4 M(1.0f);
        M = glm::translate(M, bounds.center);
        M = glm::rotate(M, a, glm::vec3(0, 1, 0));
        M = glm::rotate(M, a * 0.6f, glm::vec3(1, 0, 0));
        M = glm::translate(M, -bounds.center);
        root->setTransform(M);

        // If window resized, refit projection/view and update eye uniform
        bonsai::util::BoundsUtils::fitCameraToBounds(viewer, bounds, fovY);

        glm::vec3 eye = computeEye();
        shader->use();
        shader->setUniform("uEyePosW", eye);
        bonsai::Shader::unuse();
    }

    viewer.close();
    return 0;
}