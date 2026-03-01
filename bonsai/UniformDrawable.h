#pragma once
#include <functional>
#include "Drawable.h"

namespace bonsai {

struct RenderState;

// A Drawable that runs a callback when drawn. Useful to set uniforms/state.
class UniformDrawable : public Drawable
{
public:
    using Callback = std::function<void(const RenderState&)>;

    explicit UniformDrawable(Callback cb) : _cb(std::move(cb)) {}
    void draw(const RenderState& state) override
    {
        if (_cb) _cb(state);
    }

private:
    Callback _cb;
};

} // namespace bonsai