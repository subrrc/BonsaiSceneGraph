#include "SgiRgbLoader.h"
#include "Image.h"

#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

namespace bonsai {

static std::uint16_t readBE16(std::istream& in)
{
    std::uint8_t b[2]{};
    in.read((char*)b, 2);
    return (std::uint16_t(b[0]) << 8) | std::uint16_t(b[1]);
}

static std::uint32_t readBE32(std::istream& in)
{
    std::uint8_t b[4]{};
    in.read((char*)b, 4);
    return (std::uint32_t(b[0]) << 24) | (std::uint32_t(b[1]) << 16) | (std::uint32_t(b[2]) << 8) | std::uint32_t(b[3]);
}

static void flipRGBA(std::vector<std::uint8_t>& pixels, int w, int h)
{
    const size_t rowBytes = (size_t)w * 4u;
    std::vector<std::uint8_t> tmp(rowBytes);

    for (int y = 0; y < h / 2; ++y)
    {
        std::uint8_t* rowA = pixels.data() + (size_t)y * rowBytes;
        std::uint8_t* rowB = pixels.data() + (size_t)(h - 1 - y) * rowBytes;

        std::copy(rowA, rowA + rowBytes, tmp.data());
        std::copy(rowB, rowB + rowBytes, rowA);
        std::copy(tmp.data(), tmp.data() + rowBytes, rowB);
    }
}

static bool decodeRleRow(std::istream& in, std::uint8_t* dst, int count, int bpc)
{
    int written = 0;

    while (written < count)
    {
        int c = in.get();
        if (c == EOF) return false;

        std::uint8_t ctrl = (std::uint8_t)c;
        if (ctrl == 0) break;

        int len = int(ctrl & 0x7F);
        if (len == 0) continue;

        if (ctrl & 0x80)
        {
            // literal
            if (bpc == 1)
            {
                in.read((char*)(dst + written), len);
                if (!in) return false;
                written += len;
            }
            else
            {
                for (int i = 0; i < len; ++i)
                {
                    std::uint16_t v = readBE16(in);
                    if (!in) return false;
                    dst[written++] = (std::uint8_t)(v >> 8);
                    if (written >= count) break;
                }
            }
        }
        else
        {
            // replicate
            if (bpc == 1)
            {
                int v = in.get();
                if (v == EOF) return false;
                std::fill(dst + written, dst + std::min(written + len, count), (std::uint8_t)v);
                written += len;
            }
            else
            {
                std::uint16_t v = readBE16(in);
                if (!in) return false;
                std::uint8_t vv = (std::uint8_t)(v >> 8);
                std::fill(dst + written, dst + std::min(written + len, count), vv);
                written += len;
            }
        }
    }

    return written >= 0;
}

std::shared_ptr<Image> SgiRgbLoader::loadRGBA(const std::string& filename, bool flipY)
{
    std::ifstream in(filename, std::ios::binary);
    if (!in) return nullptr;

    const std::uint16_t magic = readBE16(in);
    if (magic != 0x01DA) return nullptr;

    const std::uint8_t storage = (std::uint8_t)in.get(); // 0 verbatim, 1 RLE
    const std::uint8_t bpc     = (std::uint8_t)in.get(); // 1 or 2
    if (bpc != 1 && bpc != 2) return nullptr;

    const std::uint16_t dimension = readBE16(in);
    const std::uint16_t xsize = readBE16(in);
    const std::uint16_t ysize = readBE16(in);
    const std::uint16_t zsize = readBE16(in);
    (void)dimension;

    (void)readBE32(in);
    (void)readBE32(in);
    (void)readBE32(in);

    in.ignore(80);
    (void)readBE32(in);

    const std::streamoff headerRead = 2 + 1 + 1 + 2 + 2 + 2 + 2 + 4 + 4 + 4 + 80 + 4;
    const std::streamoff pad = 512 - headerRead;
    if (pad > 0) in.ignore(pad);

    if (!in || xsize == 0 || ysize == 0 || zsize == 0 || zsize > 4) return nullptr;

    const int w = (int)xsize;
    const int h = (int)ysize;
    const int channels = (int)zsize;

    std::vector<std::uint8_t> rgba((size_t)w * (size_t)h * 4u, 255);

    if (storage == 0)
    {
        for (int c = 0; c < channels; ++c)
        {
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    std::uint8_t v8 = 0;
                    if (bpc == 1)
                    {
                        int v = in.get();
                        if (v == EOF) return nullptr;
                        v8 = (std::uint8_t)v;
                    }
                    else
                    {
                        std::uint16_t v = readBE16(in);
                        if (!in) return nullptr;
                        v8 = (std::uint8_t)(v >> 8);
                    }

                    size_t idx = ((size_t)y * (size_t)w + (size_t)x) * 4u;
                    if (channels == 1)
                    {
                        rgba[idx + 0] = v8; rgba[idx + 1] = v8; rgba[idx + 2] = v8;
                    }
                    else if (channels == 2)
                    {
                        if (c == 0) { rgba[idx+0]=v8; rgba[idx+1]=v8; rgba[idx+2]=v8; }
                        if (c == 1) { rgba[idx+3]=v8; }
                    }
                    else
                    {
                        rgba[idx + (size_t)c] = v8;
                    }
                }
            }
        }
    }
    else if (storage == 1)
    {
        const int rows = h * channels;
        std::vector<std::uint32_t> rowStart(rows);
        std::vector<std::uint32_t> rowSize(rows);

        for (int i = 0; i < rows; ++i) rowStart[i] = readBE32(in);
        for (int i = 0; i < rows; ++i) rowSize[i]  = readBE32(in);
        if (!in) return nullptr;

        std::vector<std::uint8_t> row(w);

        for (int c = 0; c < channels; ++c)
        {
            for (int y = 0; y < h; ++y)
            {
                const int rowIndex = c * h + y;
                const std::uint32_t off = rowStart[rowIndex];
                (void)rowSize[rowIndex];
                if (off == 0) return nullptr;

                std::streampos cur = in.tellg();
                in.seekg((std::streamoff)off, std::ios::beg);
                if (!in) return nullptr;

                if (!decodeRleRow(in, row.data(), w, bpc))
                    return nullptr;

                in.seekg(cur);

                for (int x = 0; x < w; ++x)
                {
                    const std::uint8_t v8 = row[x];
                    size_t idx = ((size_t)y * (size_t)w + (size_t)x) * 4u;

                    if (channels == 1)
                    {
                        rgba[idx + 0] = v8; rgba[idx + 1] = v8; rgba[idx + 2] = v8;
                    }
                    else if (channels == 2)
                    {
                        if (c == 0) { rgba[idx+0]=v8; rgba[idx+1]=v8; rgba[idx+2]=v8; }
                        if (c == 1) { rgba[idx+3]=v8; }
                    }
                    else
                    {
                        rgba[idx + (size_t)c] = v8;
                    }
                }
            }
        }
    }
    else
    {
        return nullptr;
    }

    if (flipY) flipRGBA(rgba, w, h);

    auto img = std::make_shared<Image>(w, h, 4);
    img->data() = std::move(rgba);
    return img;
}

} // namespace bonsai
