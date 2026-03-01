// examples/main.cpp
//
// Demonstrates shader-only multi-texturing with bonsai.
// - Creates a Viewer window (context active).
// - Builds a GLSL shader (ARB shader objects path).
// - Creates TWO small checker textures.
// - Binds them to texture units 0 and 1 on a Geometry.
// - Fragment shader mixes the two textures 50/50.
//
// NOTE: Vertex shader uses built-in attributes (gl_Vertex, gl_MultiTexCoord0)
// so it works with your existing fixed-function client arrays.

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

    // 1) Create viewer + window/context first (required for shader compilation)
    Viewer viewer;
    if (!viewer.open(800, 600, "bonsai multitexture shader example"))
        return 1;

    // 2) Build shader from source
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
        uniform sampler2D uTexture1;
        varying vec2 vUV;

        void main()
        {
            vec4 a = texture2D(uTexture0, vUV);
            vec4 b = texture2D(uTexture1, vUV);
            gl_FragColor = mix(a, b, 0.5);
        }
    )GLSL";

    auto shader = std::make_shared<Shader>();
    if (!shader->buildFromSource(vs, fs))
        return 2;

    viewer.setShader(shader);

    // 3) Create two textures (CPU-side only; they upload on first bind)
    const std::uint8_t red[4]   = {255,  0,  0, 255};
    const std::uint8_t green[4] = {  0,255,  0, 255};
    const std::uint8_t blue[4]  = {  0,  0,255, 255};
    const std::uint8_t white[4] = {255,255,255,255};

    auto tex0 = bonsai::test::TestUtils::makeCheckerWxH_RGBA(256,256, red, white);
    auto tex1 = bonsai::test::TestUtils::makeCheckerWxH_RGBA(128,128, blue, green);

    // 4) Build scene: one Geode with one Geometry triangle
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

    // Bind textures to units 0 and 1 (shader-only multitexture)
    geom->setTexture(0, tex0);
    geom->setTexture(1, tex1);

    geode->addDrawable(geom);
    viewer.setSceneData(root);

    // 5) Camera
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, 0.1f, 10.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 2),
                                 glm::vec3(0, 0, 0),
                                 glm::vec3(0, 1, 0));

    viewer.setProjectionMatrix(proj);
    viewer.setViewMatrix(view);

    // 6) Run window loop
    return viewer.run(800, 600, "bonsai multitexture shader example");
}