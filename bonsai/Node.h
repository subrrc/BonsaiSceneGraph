#pragma once
#include <memory>
#include <string>
#include <cstdint>
#include <glm/glm.hpp>
#include "RenderState.h"

namespace bonsai {

class Node : public std::enable_shared_from_this<Node>
{
public:
    Node();
    virtual ~Node() = default;

    void setName(const std::string& n) { _name = n; bumpRevision(); }
    const std::string& getName() const { return _name; }

    // Local transform relative to parent
    void setTransform(const glm::mat4& m) { _transform = m; _hasTransform = true; bumpRevision(); }
    const glm::mat4& getTransform() const { return _transform; }

    void clearTransform() { _transform = glm::mat4(1.0f); _hasTransform = false; bumpRevision(); }
    bool hasTransform() const { return _hasTransform; }

    // Revision counter used for caching/batching
    std::uint64_t revision() const { return _revision; }

    // Traversal entry point
    virtual void traverse(RenderState& state);

protected:
    // Override in subclasses that render content
    virtual void draw(RenderState& /*state*/) {}

    void bumpRevision() { ++_revision; }

private:
    std::string _name;
    glm::mat4 _transform{1.0f};
    bool _hasTransform{false};

    std::uint64_t _revision{1};
};

using NodePtr = std::shared_ptr<Node>;

} // namespace bonsai
