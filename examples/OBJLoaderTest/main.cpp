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
#include "BoundsUtils.h" // only needed for bonsai::util::Bounds type

int main(int argc, char** argv)
{
    using namespace bonsai;
    using clock_t = std::chrono::steady_clock;

    if (argc < 2)
    {
        std::cout << "Usage: rotating_obj_windowed_center_fit <model.obj>\n";
        return 0;
    }

    const std::string objPath = argv[1];

    Viewer viewer;
    if (!viewer.open(900, 700, "Rotating OBJ (loader bounds) - Windowed"))
        return 1;

    // Normal-based shading (works if loader has normals or computed flat normals)
    const std::string vs = R"GLSL(
        uniform mat4 uMVP;
        uniform mat4 uModel;
        varying vec3 vN;

        void main()
        {
            vN = normalize((uModel * vec4(gl_Normal, 0.0)).xyz);
            gl_Position = uMVP * gl_Vertex;
        }
    )GLSL";

    const std::string fs = R"GLSL(
        varying vec3 vN;
        void main()
        {
            vec3 L = normalize(vec3(0.4, 0.8, 0.6));
            float d = max(dot(normalize(vN), L), 0.0);
            vec3 base = vec3(0.85, 0.85, 0.9);
            gl_FragColor = vec4(base * (0.2 + 0.8*d), 1.0);
        }
    )GLSL";

    auto shader = std::make_shared<Shader>();
    if (!shader->buildFromSource(vs, fs))
        return 2;
    viewer.setShader(shader);

    // Load OBJ -> Group node + bounds
    ObjLoader::Options opt;
    opt.loadTexcoords = true;
    opt.loadNormals = true;
    opt.computeNormalsIfMissing = true;
    opt.flipV = false;
    opt.scale = 1.0f;

    bonsai::util::Bounds bounds;
    NodePtr modelRoot = ObjLoader::load(objPath, opt, &bounds);
    if (!modelRoot)
    {
        std::cout << "Failed to load OBJ: " << objPath << "\n";
        return 3;
    }

    // Put the model under a parent so we can animate cleanly
    auto root = std::make_shared<Group>();
    root->addChild(modelRoot);
    viewer.setSceneData(root);

    // Fit camera using bounds at FOV 40
    bonsai::util::BoundsUtils::fitCameraToBounds(viewer, bounds, 40.0f);

    // Rotate 30 deg/sec around bounds center
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

        // Keep camera/projection correct on resize
        bonsai::util::BoundsUtils::fitCameraToBounds(viewer, bounds, 40.0f);
    }

    viewer.close();
    return 0;
}