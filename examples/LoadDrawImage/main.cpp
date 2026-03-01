// examples/textured_quad_from_file.cpp
//
// Loads an image from a command-line argument and applies it to a quad.
// Uses ImageLoader (currently .png via WIC on Windows), lazy texture upload on bind,
// and a simple shader sampling uTexture0.
//
// Usage:
//   textured_quad_from_file.exe path/to/image.png

#include <memory>
#include <string>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Viewer.h"
#include "Group.h"
#include "Geode.h"
#include "Geometry.h"
#include "Shader.h"
#include "ImageLoader.h"

int main(int argc, char** argv)
{
    using namespace bonsai;

    if (argc < 2)
    {
        std::cout << "Usage: textured_quad_from_file <image.png>\n";
        return 0;
    }

    const std::string imagePath = argv[1];

    Viewer viewer;
    if (!viewer.open(900, 700, "Textured Quad (from file)"))
        return 1;

    // Shader: built-in attributes + texcoord, samples uTexture0
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

    // Load image (RGBA8), lazy uploads to GL on first bind
    auto img = bonsai::ImageLoader::loadRGBA(imagePath);
    if (!img)
    {
        std::cout << "Failed to load image: " << imagePath << "\n";
        return 3;
    }

    // Build scene: a quad made of two triangles
    auto root  = std::make_shared<Group>();
    auto geode = std::make_shared<Geode>();
    root->addChild(geode);

    auto quad = std::make_shared<Geometry>();
    quad->setPrimitive(Geometry::Primitive::Triangles);

    // Quad in XY plane centered at origin
    quad->vertices() = {
        {-1.0f, -1.0f, 0.0f},  // 0
        { 1.0f, -1.0f, 0.0f},  // 1
        { 1.0f,  1.0f, 0.0f},  // 2

        {-1.0f, -1.0f, 0.0f},  // 0
        { 1.0f,  1.0f, 0.0f},  // 2
        {-1.0f,  1.0f, 0.0f},  // 3
    };

    // UVs
    quad->texcoords() = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},

        {0.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
    };

    quad->setTexture(0, img);

    geode->addDrawable(quad);
    viewer.setSceneData(root);

    // Camera: orthographic-ish look using perspective and a close camera
    viewer.setViewMatrix(glm::lookAt(glm::vec3(0, 0, 2.5f),
                                     glm::vec3(0, 0, 0),
                                     glm::vec3(0, 1, 0)));

    auto setProjFromViewer = [&]()
    {
        const float aspect = (viewer.height() > 0) ? (float)viewer.width() / (float)viewer.height() : 1.0f;
        viewer.setProjectionMatrix(glm::perspective(glm::radians(40.0f), aspect, 0.1f, 10.0f));
    };
    setProjFromViewer();

    // Main loop
    while (viewer.frame())
    {
        // If resized, keep aspect correct
        setProjFromViewer();
    }

    viewer.close();
    return 0;
}