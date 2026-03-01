#pragma once
#include <memory>
#include <string>

namespace bonsai {
class Image;

class BmpLoader
{
public:
    // Loads BMP from disk and returns RGBA8 (optionally flipped for OpenGL UVs).
    static std::shared_ptr<Image> loadRGBA(const std::string& filename, bool flipY = true);
};

} // namespace bonsai