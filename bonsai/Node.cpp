#include "Node.h"

namespace bonsai {

Node::Node() = default;

void Node::traverse(RenderState& state)
{
    const glm::mat4 prevModel = state.model;
    if (_hasTransform)
        state.model = state.model * _transform;

    draw(state);

    state.model = prevModel;
}

} // namespace bonsai
