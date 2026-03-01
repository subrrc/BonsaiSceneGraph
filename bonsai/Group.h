#pragma once
#include <vector>
#include <cstddef>
#include "Node.h"

namespace bonsai {

class Group : public Node
{
public:
    Group() = default;
    ~Group() override = default;

    virtual void addChild(const NodePtr& child);
    virtual void removeChild(const NodePtr& child);

    // Insert at a specific index (0 inserts at front)
    virtual void insertChild(std::size_t index, const NodePtr& child);

    const std::vector<NodePtr>& getChildren() const { return _children; }

    void traverse(RenderState& state) override;

protected:
    std::vector<NodePtr> _children;
};

using GroupPtr = std::shared_ptr<Group>;

} // namespace bonsai
