#include "BoundsUtils.h"

#if defined(_WIN32)
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
#endif

#include <algorithm>
#include <limits>

#include <glm/gtc/matrix_transform.hpp>

#include "Viewer.h"
#include "Node.h"
#include "Group.h"
#include "Geode.h"
#include "Geometry.h"

namespace bonsai::util {

struct AccumBounds
{
    glm::vec3 mn{ std::numeric_limits<float>::max() };
    glm::vec3 mx{ std::numeric_limits<float>::lowest() };
    bool any{false};

    void expand(const glm::vec3& p)
    {
        mn.x = (std::min)(mn.x, p.x);
        mn.y = (std::min)(mn.y, p.y);
        mn.z = (std::min)(mn.z, p.z);

        mx.x = (std::max)(mx.x, p.x);
        mx.y = (std::max)(mx.y, p.y);
        mx.z = (std::max)(mx.z, p.z);

        any = true;
    }
};

static void accumulateNodeRecursive(const std::shared_ptr<bonsai::Node>& node,
                                   const glm::mat4& parent,
                                   AccumBounds& acc)
{
    if (!node) return;

    // Accumulate local transform if any
    glm::mat4 M = parent;
    if (node->hasTransform())
        M = parent * node->getTransform();

    // If this is a Geode, inspect drawables for Geometry
    if (auto geode = std::dynamic_pointer_cast<bonsai::Geode>(node))
    {
        const auto& ds = geode->getDrawables();
        for (const auto& d : ds)
        {
            auto geom = std::dynamic_pointer_cast<bonsai::Geometry>(d);
            if (!geom) continue;

            const auto& V = geom->vertices();
            for (const auto& p : V)
            {
                glm::vec4 wp4 = M * glm::vec4(p, 1.0f);
                acc.expand(glm::vec3(wp4));
            }
        }
    }

    // If this is a Group, traverse children
    if (auto group = std::dynamic_pointer_cast<bonsai::Group>(node))
    {
        const auto& children = group->getChildren();
        for (const auto& c : children)
            accumulateNodeRecursive(c, M, acc);
    }
}

bool BoundsUtils::computeBoundsFromNode(const std::shared_ptr<bonsai::Node>& node, Bounds& out)
{
    AccumBounds acc;
    accumulateNodeRecursive(node, glm::mat4(1.0f), acc);

    if (!acc.any)
        return false;

    out.min = acc.mn;
    out.max = acc.mx;
    out.center = (acc.mn + acc.mx) * 0.5f;

    // bounding sphere around AABB
    out.radius = glm::length(acc.mx - out.center);
    if (out.radius < 1e-6f) out.radius = 1.0f;

    return true;
}

void BoundsUtils::fitCameraToBounds(
    Viewer& viewer,
    const Bounds& b,
    float fovY_degrees,
    const glm::vec3& viewDir,
    const glm::vec3& up,
    float margin)
{
    const float aspect =
        (viewer.height() > 0) ? (float)viewer.width() / (float)viewer.height() : 1.0f;

    const float fovY = glm::radians(fovY_degrees);

    // distance to fit bounding sphere within vertical FOV
    float dist = (b.radius / std::tan(fovY * 0.5f)) * margin;

    glm::vec3 eye = b.center + glm::normalize(viewDir) * dist;

    viewer.setViewMatrix(glm::lookAt(eye, b.center, up));

    // near/far planes based on radius and distance
    float nearZ = (std::max)(0.01f, dist - b.radius * 2.0f);
    float farZ  = dist + b.radius * 2.0f;

    viewer.setProjectionMatrix(glm::perspective(fovY, aspect, nearZ, farZ));
}

} // namespace bonsai::util