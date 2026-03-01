#pragma once
#include <memory>
#include <string>

namespace bonsai {
class Image;

// SGI RGB (.rgb/.rgba/.sgi) loader into RGBA8
class SgiRgbLoader
{
public:
    static std::shared_ptr<Image> loadRGBA(const std::string& filename, bool flipY = false);
};

} // namespace bonsai
