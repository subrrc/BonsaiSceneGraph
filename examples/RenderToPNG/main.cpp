// examples/offscreen_to_png.cpp
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Viewer.h"
#include "Group.h"
#include "Geode.h"
#include "Geometry.h"
#include "Image.h"
#include "TestUtils.h"

int main()
{
    using namespace bonsai;

    // 1) Create viewer in offscreen mode (creates hidden WGL context)
    Viewer viewer;
    if (!viewer.openOffscreen(512, 512))
        return 1;

    // 3) Create two textures (CPU-side only; they upload on first bind)
    const std::uint8_t blue[4] = { 0,  0,255, 255 };
    const std::uint8_t magenta[4] = { 255,0,255,255 };

    auto img = bonsai::test::TestUtils::makeCheckerWxH_RGBA(256, 256, magenta, blue);

    // 3) Build a simple scene (textured triangle)
    auto root = std::make_shared<Group>();

    auto geom = std::make_shared<Geometry>();
    geom->setPrimitive(Geometry::Primitive::Triangles);

    geom->vertices() = {
        {-0.75f, -0.75f, 0.0f},
        { 0.75f, -0.75f, 0.0f},
        { 0.00f,  0.75f, 0.0f},
    };

    geom->texcoords() = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.5f, 1.0f},
    };

    geom->setTexture(img);

    auto geode = std::make_shared<Geode>();
    geode->addDrawable(geom);
    root->addChild(geode);

    viewer.setSceneData(root);

    // 4) Set camera
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 2),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0));
    viewer.setProjectionMatrix(proj);
    viewer.setViewMatrix(view);

    // 5) Render once and save PNG
    viewer.renderOnce();
    if (!viewer.savePNG("offscreen.png"))
        return 2;

    viewer.close();
    return 0;
}
