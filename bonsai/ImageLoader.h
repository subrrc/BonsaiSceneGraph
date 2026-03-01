#pragma once
#include <memory>
#include <string>

namespace bonsai {

class Image;

// Dispatches to format-specific loaders based on file extension.
// Currently supports: .png (Windows/WIC via PngLoader)
class ImageLoader
{
public:
    // Loads image as RGBA8. Returns nullptr on failure or unsupported format.
    static std::shared_ptr<Image> loadRGBA(const std::string& filename);

    // Utility: returns lowercase file extension without dot, e.g. "png".
    static std::string extensionLower(const std::string& filename);
};

} // namespace bonsai