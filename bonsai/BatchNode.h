#pragma once
#include <vector>
#include <cstdint>
#include <memory>
#include <glm/glm.hpp>

#include "Group.h"

namespace bonsai {

class Geometry;

class BatchNode : public Group
{
public:
    BatchNode() = default;
    ~BatchNode() override;

    void markDirty();

    void setIncludeTextured(bool v) { _includeTextured = v; markDirty(); }
    void setIncludeUntextured(bool v) { _includeUntextured = v; markDirty(); }

    void traverse(RenderState& state) override;

    void addChild(const NodePtr& child) override;
    void removeChild(const NodePtr& child) override;
    void insertChild(std::size_t index, const NodePtr& child) override;

private:
    struct Item
    {
        std::shared_ptr<Geometry> geom;
        glm::mat4 localModel{1.0f};
    };

    void rebuildIfNeeded();
    void rebuildDisplayList();
    std::uint64_t computeSubtreeHash() const;
    void collectGeometry(const NodePtr& node, const glm::mat4& parentModel, std::vector<Item>& out) const;

private:
    unsigned int _listId{0};
    bool _dirty{true};
    std::uint64_t _compiledHash{0};

    bool _includeTextured{true};
    bool _includeUntextured{true};
};

using BatchNodePtr = std::shared_ptr<BatchNode>;

} // namespace bonsai
