#pragma once
#include <glm/glm.hpp>

namespace bonsai {

class Shader; // NEW forward declaration

// Passed down during traversal/draw.
struct RenderState
{
    glm::mat4 view{1.0f};
    glm::mat4 proj{1.0f};
    glm::mat4 model{1.0f}; // accumulated model matrix

    Shader* shader{nullptr}; // NEW: optional active shader
};

} // namespace bonsai
