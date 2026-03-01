#include <memory>
#include <random>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Viewer.h"
#include "BatchNode.h"
#include "Group.h"
#include "Geode.h"
#include "Geometry.h"
#include "Shader.h"

int main()
{
    using namespace bonsai;

    Viewer viewer;
    if (!viewer.open(1000, 800, "BatchNode Quad Grid 100x100"))
        return 1;

    // Simple shader: pass vertex color
    const std::string vs = R"GLSL(
        uniform mat4 uMVP;
        varying vec4 vColor;
        void main() {
            vColor = gl_Color;
            gl_Position = uMVP * gl_Vertex;
        }
    )GLSL";

    const std::string fs = R"GLSL(
        varying vec4 vColor;
        void main() { gl_FragColor = vColor; }
    )GLSL";

    auto shader = std::make_shared<Shader>();
    if (!shader->buildFromSource(vs, fs))
        return 2;
    viewer.setShader(shader);

    auto root  = std::make_shared<Group>();
    auto batch = std::make_shared<BatchNode>();
    root->addChild(batch);

    // Random colors
    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    const int W = 100;
    const int H = 100;

    // World size: fit into [-1,1] in X and Y
    const float sx = 2.0f / float(W);
    const float sy = 2.0f / float(H);

    for (int y = 0; y < H; ++y)
    {
        for (int x = 0; x < W; ++x)
        {
            const float x0 = -1.0f + x * sx;
            const float y0 = -1.0f + y * sy;
            const float x1 = x0 + sx * 0.95f;
            const float y1 = y0 + sy * 0.95f;

            const glm::vec4 c(dist(rng), dist(rng), dist(rng), 1.0f);

            auto geode = std::make_shared<Geode>();
            auto quad  = std::make_shared<Geometry>();
            quad->setPrimitive(Geometry::Primitive::Triangles);

        quad->setVertices({
            {x0, y0, 0.0f}, {x1, y0, 0.0f}, {x1, y1, 0.0f},
            {x0, y0, 0.0f}, {x1, y1, 0.0f}, {x0, y1, 0.0f},
        });

        quad->setColors({ c, c, c, c, c, c });

            geode->addDrawable(quad);
            batch->addChild(geode);
        }
    }

    viewer.setSceneData(root);

    // Camera
    viewer.setViewMatrix(glm::lookAt(glm::vec3(0, 0, 2.0f),
                                     glm::vec3(0, 0, 0),
                                     glm::vec3(0, 1, 0)));

    auto setProj = [&]() {
        float aspect = (viewer.height() > 0) ? float(viewer.width()) / float(viewer.height()) : 1.0f;
        viewer.setProjectionMatrix(glm::perspective(glm::radians(40.0f), aspect, 0.1f, 10.0f));
    };
    setProj();

    while (viewer.frame())
    {
        setProj();
        // The BatchNode will rebuild only once (unless you modify the subtree).
    }

    viewer.close();
    return 0;
}
