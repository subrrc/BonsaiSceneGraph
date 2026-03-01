#pragma once
#include <memory>
#include <string>

namespace bonsai {
class Image;

// Windows-only helper that decodes via WIC into RGBA8 and flips vertically (OpenGL UV convention).
class WicImageLoader
{
public:
    static std::shared_ptr<Image> loadRGBA(const std::string& filename, bool flipY = true);
};

} // namespace bonsai
