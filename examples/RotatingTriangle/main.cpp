// examples/rotating_triangle_windowed.cpp
//
// Windowed Viewer example (Viewer owns Win32/WGL window + context).
// Renders a textured triangle rotating about its center at 10 degrees per second.
//
// Requires:
// - Updated Viewer with frame() public method
// - Shader class (ARB shader objects)
// - Geometry multi-texture update (we only use unit 0 here)
// - Image lazy upload (bind uploads when dirty)
// - TestUtils::makeCheckerWxH_RGBA (shared sample utility)

#include <memory>
#include <string>
#include <cstdint>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Viewer.h"
#include "Group.h"
#include "Geode.h"
#include "Geometry.h"
#include "Shader.h"
#include "TestUtils.h"

int main()
{
    using namespace bonsai;
    using clock_t = std::chrono::steady_clock;

    // 1) Create and open Viewer window/context
    Viewer viewer;
    if (!viewer.open(800, 600, "Rotating Triangle (10 deg/sec) - Windowed"))
        return 1;

    // 2) Build GLSL shader (uses built-in attributes so it works with client arrays)
    const std::string vs = R"GLSL(
        uniform mat4 uMVP;
        varying vec2 vUV;

        void main()
        {
            vUV = gl_MultiTexCoord0.xy;
            gl_Position = uMVP * gl_Vertex;
        }
    )GLSL";

    const std::string fs = R"GLSL(
        uniform sampler2D uTexture0;
        varying vec2 vUV;

        void main()
        {
            gl_FragColor = texture2D(uTexture0, vUV);
        }
    )GLSL";

    auto shader = std::make_shared<Shader>();
    if (!shader->buildFromSource(vs, fs))
        return 2;

    viewer.setShader(shader);

    // 3) Build scene graph
    auto root  = std::make_shared<Group>();
    auto geode = std::make_shared<Geode>();
    root->addChild(geode);

    auto geom = std::make_shared<Geometry>();
    geom->setPrimitive(Geometry::Primitive::Triangles);

    // Triangle roughly centered around origin -> rotation about origin looks like "about its center"
    geom->vertices() = {
        {-0.6f, -0.5f, 0.0f},
        { 0.6f, -0.5f, 0.0f},
        { 0.0f,  0.7f, 0.0f},
    };

    geom->texcoords() = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.5f, 1.0f},
    };

    // Checker texture (CPU-side only; uploads lazily on first bind)
    const std::uint8_t a[4] = {255, 255, 255, 255};
    const std::uint8_t b[4] = { 40, 160, 255, 255};
    auto tex = bonsai::test::TestUtils::makeCheckerWxH_RGBA(256, 256, a, b);

    geom->setTexture(0, tex);
    geode->addDrawable(geom);

    viewer.setSceneData(root);

    // 4) Camera
    glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 2.0f),
                                 glm::vec3(0, 0, 0),
                                 glm::vec3(0, 1, 0));
    viewer.setViewMatrix(view);

    // Projection will be updated each frame for correct aspect
    auto setProjFromViewer = [&]()
    {
        const float aspect = (viewer.height() > 0) ? (float)viewer.width() / (float)viewer.height() : 1.0f;
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 10.0f);
        viewer.setProjectionMatrix(proj);
    };
    setProjFromViewer();

    // 5) Animation timing: 10 degrees per second
    const float degPerSec = 10.0f;
    const auto t0 = clock_t::now();

    // 6) Main loop: poll events + render one frame via viewer.frame()
    while (viewer.frame())
    {
        const auto t1 = clock_t::now();
        const float seconds = std::chrono::duration<float>(t1 - t0).count();

        const float angleRad = glm::radians(degPerSec * seconds);

        // Rotate geode about Z axis around origin
        geode->setTransform(glm::rotate(glm::mat4(1.0f), angleRad, glm::vec3(0, 0, 1)));

        // Keep projection correct if the window was resized
        setProjFromViewer();
    }

    // viewer.frame() will close internally when needed; calling close() is fine too
    viewer.close();
    return 0;
}