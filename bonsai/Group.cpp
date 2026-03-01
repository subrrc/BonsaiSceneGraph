#include "Group.h"
#include <algorithm>

namespace bonsai {

void Group::addChild(const NodePtr& child)
{
    if (!child) return;
    _children.push_back(child);
    bumpRevision();
}

void Group::removeChild(const NodePtr& child)
{
    const auto before = _children.size();

    _children.erase(
        std::remove(_children.begin(), _children.end(), child),
        _children.end()
    );

    if (_children.size() != before)
        bumpRevision();
}

void Group::insertChild(std::size_t index, const NodePtr& child)
{
    if (!child) return;

    if (index >= _children.size())
        _children.push_back(child);
    else
        _children.insert(_children.begin() + static_cast<std::ptrdiff_t>(index), child);

    bumpRevision();
}

void Group::traverse(RenderState& state)
{
    const glm::mat4 prevModel = state.model;
    if (hasTransform())
        state.model = state.model * getTransform();

    draw(state);

    for (auto& c : _children)
        if (c) c->traverse(state);

    state.model = prevModel;
}

} // namespace bonsai
