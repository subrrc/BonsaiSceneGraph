#pragma once
#include <vector>
#include "Node.h"
#include "Drawable.h"

namespace bonsai {

// Like OSG Geode: a node that contains Drawables.
class Geode : public Node
{
public:
    Geode() = default;
    ~Geode() override = default;

    void addDrawable(const DrawablePtr& d);
    void removeDrawable(const DrawablePtr& d);

    const std::vector<DrawablePtr>& getDrawables() const { return _drawables; }

protected:
    void draw(RenderState& state) override;

private:
    std::vector<DrawablePtr> _drawables;
};

using GeodePtr = std::shared_ptr<Geode>;

} // namespace bonsai
