// examples/shader_textured_triangle.cpp
//
// Builds a simple shader (GLSL) at runtime, uploads a tiny checker texture,
// and draws a textured triangle using bonsai::Geometry + bonsai::Viewer.
//
// NOTE: This example uses built-in GLSL attributes (gl_Vertex, gl_MultiTexCoord0)
// so it works with your existing fixed-function client arrays (glVertexPointer,
// glTexCoordPointer) without any extra attribute binding code.

#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Viewer.h"
#include "Group.h"
#include "Geode.h"
#include "Geometry.h"
#include "Image.h"
#include "Shader.h"
#include "TestUtils.h"

int main()
{
    using namespace bonsai;

    // 1) Create window/context first so shader + texture can compile/upload
    Viewer viewer;
    if (!viewer.open(800, 600, "bonsai shader textured triangle"))
        return 1;

    // 2) Build shader from source (uses built-in attributes)
    const std::string vs = R"GLSL(
        // GLSL 1.10 style (typical for ARB shader objects)
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

    // 3) Create two textures (CPU-side only; they upload on first bind)
    const std::uint8_t blue[4] = { 0,  0,255, 255 };
    const std::uint8_t white[4] = { 255,255,255,255 };

    auto img = bonsai::test::TestUtils::makeCheckerWxH_RGBA(256,256,white,blue);

    // 4) Build scene: one Geode with a Geometry (triangle)
    auto root  = std::make_shared<Group>();
    auto geode = std::make_shared<Geode>();
    root->addChild(geode);

    auto geom = std::make_shared<Geometry>();
    geom->setPrimitive(Geometry::Primitive::Triangles);

    geom->vertices() = {
        {-0.8f, -0.8f, 0.0f},
        { 0.8f, -0.8f, 0.0f},
        { 0.0f,  0.8f, 0.0f},
    };

    geom->texcoords() = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.5f, 1.0f},
    };

    // Optional color array; if omitted, Geometry sets glColor(1,1,1,1)
    // geom->colors() = { {1,1,1,1}, {1,1,1,1}, {1,1,1,1} };

    geom->setTexture(img);
    geode->addDrawable(geom);

    viewer.setSceneData(root);

    // 5) Camera (simple perspective)
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, 0.1f, 10.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 2),
                                 glm::vec3(0, 0, 0),
                                 glm::vec3(0, 1, 0));
    viewer.setProjectionMatrix(proj);
    viewer.setViewMatrix(view);

    // 6) Run the normal window loop
    return viewer.run(800, 600, "bonsai shader textured triangle");
}