#pragma once
#include <memory>
#include <string>

namespace bonsai {
class Image;

class TiffLoader
{
public:
    static std::shared_ptr<Image> loadRGBA(const std::string& filename, bool flipY = true);
};

} // namespace bonsai
