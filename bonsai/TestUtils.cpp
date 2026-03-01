#include "TestUtils.h"
#include "Image.h"

#include <vector>

#if defined(_WIN32)
#include <windows.h> // for UINT/UINT8 types in your original code
#endif

namespace bonsai::test {

    std::shared_ptr<bonsai::Image> TestUtils::makeCheckerWxH_RGBA(
        std::uint32_t width,
        std::uint32_t height,
        const std::uint8_t a[4],
        const std::uint8_t b[4])
    {
        auto img = std::make_shared<bonsai::Image>((int)width, (int)height, 4);

        constexpr std::uint32_t TexturePixelSize = 4;

        const std::uint32_t rowPitch = width * TexturePixelSize;
        const std::uint32_t cellPitch = rowPitch >> 3;   // width of a cell in bytes
        const std::uint32_t cellHeight = height >> 3;   // height of a cell in pixels
        const std::uint32_t textureSize = rowPitch * height;

        // Avoid division-by-zero for small textures (< 8)
        const std::uint32_t safeCellPitch = (cellPitch == 0) ? TexturePixelSize : cellPitch;
        const std::uint32_t safeCellHeight = (cellHeight == 0) ? 1u : cellHeight;

        std::vector<std::uint8_t> data(textureSize);
        std::uint8_t* pData = data.data();

        for (std::uint32_t n = 0; n < textureSize; n += TexturePixelSize)
        {
            const std::uint32_t x = n % rowPitch;
            const std::uint32_t y = n / rowPitch;

            const std::uint32_t i = x / safeCellPitch;
            const std::uint32_t j = y / safeCellHeight;

            const std::uint8_t* src = ((i % 2) == (j % 2)) ? a : b;

            pData[n + 0] = src[0]; // R
            pData[n + 1] = src[1]; // G
            pData[n + 2] = src[2]; // B
            pData[n + 3] = src[3]; // A
        }

        // Marks dirty automatically (your Image::data() setter does that)
        img->data() = std::move(data);

        // No need to call uploadToGL(); Image uploads lazily on bind().
        return img;
    }

} // namespace bonsai::test