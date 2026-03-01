#pragma once
#include <memory>

namespace bonsai {

struct RenderState;

class Drawable
{
public:
    virtual ~Drawable() = default;
    virtual void draw(const RenderState& state) = 0;
};

using DrawablePtr = std::shared_ptr<Drawable>;

} // namespace bonsai
