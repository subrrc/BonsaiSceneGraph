#pragma once
#include <memory>
#include <string>

namespace bonsai {
class Image;

class JpegLoader
{
public:
    static std::shared_ptr<Image> loadRGBA(const std::string& filename, bool flipY = true);
};

} // namespace bonsai
