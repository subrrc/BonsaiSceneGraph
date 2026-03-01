#include "TiffLoader.h"
#include "WicImageLoader.h"

namespace bonsai {

std::shared_ptr<Image> TiffLoader::loadRGBA(const std::string& filename, bool flipY)
{
    return WicImageLoader::loadRGBA(filename, flipY);
}

} // namespace bonsai
