// examples/rotating_rgb_cube_windowed.cpp
//
// Windowed Viewer example (Viewer owns Win32/WGL window + context).
// Draws a 3D cube and rotates it. Faces are colored red/green/blue (paired faces share color).
// FOV is set to 40 degrees.
//
// Uses fixed-function vertex colors (glColorPointer) and a very simple shader that
// passes gl_Color through, so it works with your current Shader/Geometry setup.

#include <memory>
#include <string>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Viewer.h"
#include "Group.h"
#include "Geode.h"
#include "Geometry.h"
#include "Shader.h"

int main()
{
    using namespace bonsai;
    using clock_t = std::chrono::steady_clock;

    Viewer viewer;
    if (!viewer.open(900, 700, "Rotating RGB Cube - FOV 40"))
        return 1;

    // Shader: built-in attributes + built-in color
    const std::string vs = R"GLSL(
        uniform mat4 uMVP;
        varying vec4 vColor;

        void main()
        {
            vColor = gl_Color;          // comes from glColorPointer / fixed pipeline color array
            gl_Position = uMVP * gl_Vertex;
        }
    )GLSL";

    const std::string fs = R"GLSL(
        varying vec4 vColor;
        void main()
        {
            gl_FragColor = vColor;
        }
    )GLSL";

    auto shader = std::make_shared<Shader>();
    if (!shader->buildFromSource(vs, fs))
        return 2;
    viewer.setShader(shader);

    // Scene graph
    auto root  = std::make_shared<Group>();
    auto geode = std::make_shared<Geode>();
    root->addChild(geode);

    auto cube = std::make_shared<Geometry>();
    cube->setPrimitive(Geometry::Primitive::Triangles);

    // Build a cube with 24 unique vertices (4 per face) so each face can have its own color.
    // Cube centered at origin, size 1 (extent 0.5)
    const float s = 0.5f;

    // Face colors (RGBA)
    const glm::vec4 RED   = {1.f, 0.f, 0.f, 1.f};
    const glm::vec4 GREEN = {0.f, 1.f, 0.f, 1.f};
    const glm::vec4 BLUE  = {0.f, 0.f, 1.f, 1.f};

    // Positions per face (4 corners each), winding CCW as seen from outside
    // +X (right)  -> RED
    glm::vec3 px0{ s, -s, -s}, px1{ s,  s, -s}, px2{ s,  s,  s}, px3{ s, -s,  s};
    // -X (left)   -> RED
    glm::vec3 nx0{-s, -s,  s}, nx1{-s,  s,  s}, nx2{-s,  s, -s}, nx3{-s, -s, -s};
    // +Y (top)    -> GREEN
    glm::vec3 py0{-s,  s, -s}, py1{-s,  s,  s}, py2{ s,  s,  s}, py3{ s,  s, -s};
    // -Y (bottom) -> GREEN
    glm::vec3 ny0{-s, -s,  s}, ny1{-s, -s, -s}, ny2{ s, -s, -s}, ny3{ s, -s,  s};
    // +Z (front)  -> BLUE
    glm::vec3 pz0{-s, -s,  s}, pz1{ s, -s,  s}, pz2{ s,  s,  s}, pz3{-s,  s,  s};
    // -Z (back)   -> BLUE
    glm::vec3 nz0{ s, -s, -s}, nz1{-s, -s, -s}, nz2{-s,  s, -s}, nz3{ s,  s, -s};

    auto& V = cube->vertices();
    auto& C = cube->colors();
    V.reserve(24);
    C.reserve(24);

    auto pushFace = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, const glm::vec4& col)
    {
        // 4 vertices
        V.push_back(a); V.push_back(b); V.push_back(c); V.push_back(d);
        C.push_back(col); C.push_back(col); C.push_back(col); C.push_back(col);
    };

    pushFace(px0, px1, px2, px3, RED);
    pushFace(nx0, nx1, nx2, nx3, RED);
    pushFace(py0, py1, py2, py3, GREEN);
    pushFace(ny0, ny1, ny2, ny3, GREEN);
    pushFace(pz0, pz1, pz2, pz3, BLUE);
    pushFace(nz0, nz1, nz2, nz3, BLUE);

    // Indices: two triangles per face (0,1,2) (0,2,3) for each quad
    auto& I = cube->indices();
    I.reserve(36);
    for (unsigned int f = 0; f < 6; ++f)
    {
        unsigned int base = f * 4;
        I.push_back(base + 0);
        I.push_back(base + 1);
        I.push_back(base + 2);

        I.push_back(base + 0);
        I.push_back(base + 2);
        I.push_back(base + 3);
    }

    geode->addDrawable(cube);
    viewer.setSceneData(root);

    // Camera
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.2f),
                                 glm::vec3(0.0f, 0.0f, 0.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    viewer.setViewMatrix(view);

    auto setProjFromViewer = [&]()
    {
        const float aspect = (viewer.height() > 0) ? (float)viewer.width() / (float)viewer.height() : 1.0f;
        glm::mat4 proj = glm::perspective(glm::radians(40.0f), aspect, 0.1f, 50.0f);
        viewer.setProjectionMatrix(proj);
    };
    setProjFromViewer();

    // Rotation speed (degrees per second)
    const float degPerSec = 30.0f;
    const auto t0 = clock_t::now();

    while (viewer.frame())
    {
        const float seconds = std::chrono::duration<float>(clock_t::now() - t0).count();
        const float a = glm::radians(degPerSec * seconds);

        // Rotate about center: Y then X for a nice motion
        glm::mat4 M(1.0f);
        M = glm::rotate(M, a, glm::vec3(0, 1, 0));
        M = glm::rotate(M, a * 0.7f, glm::vec3(1, 0, 0));
        geode->setTransform(M);

        setProjFromViewer();
    }

    viewer.close();
    return 0;
}