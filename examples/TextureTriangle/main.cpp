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
    bonsai::Viewer viewer;
    viewer.open(800, 600, "bonsai");

    auto root = std::make_shared<Group>();

    // A textured triangle
    auto geo = std::make_shared<Geometry>();
    geo->setPrimitive(Geometry::Primitive::Triangles);

    geo->vertices() = {
        { -0.5f, -0.5f, 0.0f },
        {  0.5f, -0.5f, 0.0f },
        {  0.0f,  0.5f, 0.0f }
    };

    geo->texcoords() = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.5f, 1.0f}
    };

    // 3) Create two textures (CPU-side only; they upload on first bind)
    const std::uint8_t blue[4] = { 0,  0,255, 255 };
    const std::uint8_t grey[4] = { 128,128,128,255 };

    auto img = bonsai::test::TestUtils::makeCheckerWxH_RGBA(256, 256, grey, blue);
  
    //img->uploadToGL();
    geo->setTexture(img);

    auto geode = std::make_shared<Geode>();
    geode->addDrawable(geo);
    root->addChild(geode);

    glm::mat4 proj = glm::perspective(glm::radians(60.0f), 800.0f/600.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0,0,2), glm::vec3(0,0,0), glm::vec3(0,1,0));

    viewer.setProjectionMatrix(proj);
    viewer.setViewMatrix(view);
    viewer.setSceneData(root);
    viewer.setClearColor(glm::vec4(0.0f, 0.8f, 0.0f, 1.0f));

    return viewer.run(800, 600, "BonsaiSceneGraph viewer");
}
