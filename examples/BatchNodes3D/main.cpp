// examples/batched_quad_volume_planes.cpp
//
// BatchNode example:
// Builds a 3D "volume" using 3 sets of cubic planes of quads:
//   - XY planes at Z slices
//   - XZ planes at Y slices
//   - YZ planes at X slices
//
// Then rotates the entire volume in 3D.
//
// NOTE: Full 100x100x100 planes is extremely large (millions of quads).
// Use STEP > 1 to reduce density.
//
// Usage: just run.
//
// Requirements:
// - Viewer with frame()
// - BatchNode
// - Geometry revision tracking enabled
// - Basic Shader support

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

static void addQuadXY(bonsai::BatchNode& batch,
                      float x0, float y0, float x1, float y1, float z,
                      const glm::vec4& c)
{
    auto geode = std::make_shared<bonsai::Geode>();
    auto quad  = std::make_shared<bonsai::Geometry>();
    quad->setPrimitive(bonsai::Geometry::Primitive::Triangles);

    quad->vertices() = {
        {x0, y0, z}, {x1, y0, z}, {x1, y1, z},
        {x0, y0, z}, {x1, y1, z}, {x0, y1, z},
    };

    quad->colors() = { c,c,c,c,c,c };
    quad->markDirty();

    geode->addDrawable(quad);
    batch.addChild(geode);
}

static void addQuadXZ(bonsai::BatchNode& batch,
                      float x0, float z0, float x1, float z1, float y,
                      const glm::vec4& c)
{
    auto geode = std::make_shared<bonsai::Geode>();
    auto quad  = std::make_shared<bonsai::Geometry>();
    quad->setPrimitive(bonsai::Geometry::Primitive::Triangles);

    quad->vertices() = {
        {x0, y, z0}, {x1, y, z0}, {x1, y, z1},
        {x0, y, z0}, {x1, y, z1}, {x0, y, z1},
    };

    quad->colors() = { c,c,c,c,c,c };
    quad->markDirty();
    geode->addDrawable(quad);
    batch.addChild(geode);
}

static void addQuadYZ(bonsai::BatchNode& batch,
                      float y0, float z0, float y1, float z1, float x,
                      const glm::vec4& c)
{
    auto geode = std::make_shared<bonsai::Geode>();
    auto quad  = std::make_shared<bonsai::Geometry>();
    quad->setPrimitive(bonsai::Geometry::Primitive::Triangles);

    quad->vertices() = {
        {x, y0, z0}, {x, y1, z0}, {x, y1, z1},
        {x, y0, z0}, {x, y1, z1}, {x, y0, z1},
    };

    quad->colors() = { c,c,c,c,c,c };
    quad->markDirty();
    geode->addDrawable(quad);
    batch.addChild(geode);
}

int main()
{
    using namespace bonsai;
    using clock_t = std::chrono::steady_clock;

    Viewer viewer;
    if (!viewer.open(1200, 900, "BatchNode Volume Planes (rotating)"))
        return 1;

    // Simple vertex-color shader
    const std::string vs = R"GLSL(
        uniform mat4 uMVP;
        varying vec4 vColor;
        void main() { vColor = gl_Color; gl_Position = uMVP * gl_Vertex; }
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

    // Density controls
    const int N = 100;      // nominal resolution per axis
    const int STEP = 1;     // change to 1 for full density (VERY heavy!)
    const int slices = (N + STEP - 1) / STEP;

    // World cube from [-1,1]
    const float minP = -1.0f;
    const float maxP =  1.0f;
    const float size = (maxP - minP);

    // One plane contains N×N quads but we can also thin the plane quads if needed.
    // Here we keep 100×100 per plane, but thin the number of planes by STEP.
    const float cell = size / float(N);

    // Random colors
    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    // Build planes:
    // XY planes at varying Z
    for (int zi = 0; zi < N; zi += STEP)
    {
        float z = minP + (zi + 0.5f) * cell;
        for (int y = 0; y < N; ++y)
        {
            for (int x = 0; x < N; ++x)
            {
                float x0 = minP + x * cell;
                float y0 = minP + y * cell;
                float x1 = x0 + cell * 0.98f;
                float y1 = y0 + cell * 0.98f;

                glm::vec4 c(dist(rng), dist(rng), dist(rng), 1.0f);
                addQuadXY(*batch, x0, y0, x1, y1, z, c);
            }
        }
    }

    // XZ planes at varying Y
    for (int yi = 0; yi < N; yi += STEP)
    {
        float y = minP + (yi + 0.5f) * cell;
        for (int z = 0; z < N; ++z)
        {
            for (int x = 0; x < N; ++x)
            {
                float x0 = minP + x * cell;
                float z0 = minP + z * cell;
                float x1 = x0 + cell * 0.98f;
                float z1 = z0 + cell * 0.98f;

                glm::vec4 c(dist(rng), dist(rng), dist(rng), 1.0f);
                addQuadXZ(*batch, x0, z0, x1, z1, y, c);
            }
        }
    }

    // YZ planes at varying X
    for (int xi = 0; xi < N; xi += STEP)
    {
        float x = minP + (xi + 0.5f) * cell;
        for (int z = 0; z < N; ++z)
        {
            for (int y = 0; y < N; ++y)
            {
                float y0 = minP + y * cell;
                float z0 = minP + z * cell;
                float y1 = y0 + cell * 0.98f;
                float z1 = z0 + cell * 0.98f;

                glm::vec4 c(dist(rng), dist(rng), dist(rng), 1.0f);
                addQuadYZ(*batch, y0, z0, y1, z1, x, c);
            }
        }
    }

    viewer.setSceneData(root);

    // Camera
    viewer.setViewMatrix(glm::lookAt(glm::vec3(0, 0, 4.0f),
                                     glm::vec3(0, 0, 0),
                                     glm::vec3(0, 1, 0)));

    auto setProj = [&]() {
        float aspect = (viewer.height() > 0) ? float(viewer.width()) / float(viewer.height()) : 1.0f;
        viewer.setProjectionMatrix(glm::perspective(glm::radians(40.0f), aspect, 0.1f, 50.0f));
    };
    setProj();

    // Animate rotation: rotate root so BatchNode only compiles once
    const auto t0 = clock_t::now();
    while (viewer.frame())
    {
        setProj();

        float seconds = std::chrono::duration<float>(clock_t::now() - t0).count();
        float ay = glm::radians(20.0f * seconds);
        float ax = glm::radians(13.0f * seconds);
        float az = glm::radians(7.0f  * seconds);

        glm::mat4 M(1.0f);
        M = glm::rotate(M, ay, glm::vec3(0, 1, 0));
        M = glm::rotate(M, ax, glm::vec3(1, 0, 0));
        M = glm::rotate(M, az, glm::vec3(0, 0, 1));
        root->setTransform(M);
    }

    viewer.close();
    return 0;
}