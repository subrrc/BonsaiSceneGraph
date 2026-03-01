#pragma once
#include <memory>
#include <string>

namespace bonsai {

class Image;

// Windows-only PNG loader using WIC.
class PngLoader
{
public:
    // Loads PNG from disk and returns an bonsai::Image in RGBA8.
    // Returns nullptr on failure.
    static std::shared_ptr<Image> loadRGBA(const std::string& filename);
};

} // namespace bonsai