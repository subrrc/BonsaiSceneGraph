#include "Geode.h"
#include <algorithm>

namespace bonsai {

void Geode::addDrawable(const DrawablePtr& d)
{
    if (!d) return;
    _drawables.push_back(d);
    bumpRevision();
}

void Geode::removeDrawable(const DrawablePtr& d)
{
    const auto before = _drawables.size();

    _drawables.erase(
        std::remove(_drawables.begin(), _drawables.end(), d),
        _drawables.end()
    );

    if (_drawables.size() != before)
        bumpRevision();
}

void Geode::draw(RenderState& state)
{
    for (auto& d : _drawables)
        if (d) d->draw(state);
}

} // namespace bonsai
