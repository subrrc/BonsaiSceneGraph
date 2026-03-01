#include "ImageLoader.h"

#include <algorithm>
#include <cctype>

#include "PngLoader.h"
#include "JpegLoader.h"
#include "TiffLoader.h"
#include "SgiRgbLoader.h"
#include "BmpLoader.h"

namespace bonsai {

    std::string ImageLoader::extensionLower(const std::string& filename)
    {
        const auto slash = filename.find_last_of("/\\");
        const auto dot = filename.find_last_of('.');

        if (dot == std::string::npos) return {};
        if (slash != std::string::npos && dot < slash) return {};

        std::string ext = filename.substr(dot + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return (char)std::tolower(c); });
        return ext;
    }

    std::shared_ptr<Image> ImageLoader::loadRGBA(const std::string& filename)
    {
        const std::string ext = extensionLower(filename);

        if (ext == "png")
            return PngLoader::loadRGBA(filename);

        if (ext == "jpg" || ext == "jpeg")
            return JpegLoader::loadRGBA(filename);

        if (ext == "tif" || ext == "tiff")
            return TiffLoader::loadRGBA(filename);

        if (ext == "bmp")
            return BmpLoader::loadRGBA(filename);

        if (ext == "rgb" || ext == "rgba" || ext == "sgi")
            return SgiRgbLoader::loadRGBA(filename);

        return nullptr;
    }

} // namespace bonsai
