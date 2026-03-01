#pragma once
#include <cstdint>
#include <memory>

namespace bonsai { class Image; }

namespace bonsai::test {

    class TestUtils
    {
    public:
        // Creates an RGBA checkerboard image (CPU-side only; uploads lazily on bind()).
        static std::shared_ptr<bonsai::Image> makeCheckerWxH_RGBA(
            std::uint32_t width,
            std::uint32_t height,
            const std::uint8_t a[4],
            const std::uint8_t b[4]);
    };

} // namespace bonsai::test