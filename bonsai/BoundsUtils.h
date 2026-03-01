#pragma once
#include <memory>
#include <glm/glm.hpp>

namespace bonsai {

class Viewer;
class Node;

namespace util {

struct Bounds
{
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
    glm::vec3 center{0.0f};
    float radius{0.0f}; // bounding sphere radius about center
};

class BoundsUtils
{
public:
    // Computes bounds for a node subtree:
    // - Traverses Group children recursively
    // - Reads Geometry vertices from Geode drawables
    // - Applies Node transforms (accumulated) while computing bounds
    static bool computeBoundsFromNode(const std::shared_ptr<Node>& node, Bounds& out);

    // Fits the viewer camera to bounds using vertical FOV in degrees.
    static void fitCameraToBounds(
        Viewer& viewer,
        const Bounds& b,
        float fovY_degrees,
        const glm::vec3& viewDir = glm::vec3(0, 0, 1),
        const glm::vec3& up = glm::vec3(0, 1, 0),
        float margin = 1.25f);
};

} // namespace util
} // namespace bonsai