#include "WicImageLoader.h"
#include "Image.h"

#if !defined(_WIN32)
  #error "WicImageLoader.cpp supports Windows only."
#endif

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
  #define NOMINMAX
#endif

#include <windows.h>
#include <wincodec.h>
#include <combaseapi.h>

#include <vector>
#include <cstdint>
#include <cstring>

#pragma comment(lib, "windowscodecs.lib")

namespace bonsai {

static std::wstring toWide(const std::string& s)
{
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (len <= 0)
    {
        len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), nullptr, 0);
        if (len <= 0) return {};
        std::wstring w((size_t)len, L'\0');
        MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), w.data(), len);
        return w;
    }
    std::wstring w((size_t)len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), len);
    return w;
}

static bool ensureCOM()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    return SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
}

static void flipRGBA(std::vector<std::uint8_t>& pixels, UINT w, UINT h)
{
    const size_t rowBytes = (size_t)w * 4u;
    std::vector<std::uint8_t> tmp(rowBytes);

    for (UINT y = 0; y < h / 2; ++y)
    {
        std::uint8_t* rowA = pixels.data() + (size_t)y * rowBytes;
        std::uint8_t* rowB = pixels.data() + (size_t)(h - 1 - y) * rowBytes;

        std::memcpy(tmp.data(), rowA, rowBytes);
        std::memcpy(rowA, rowB, rowBytes);
        std::memcpy(rowB, tmp.data(), rowBytes);
    }
}

std::shared_ptr<Image> WicImageLoader::loadRGBA(const std::string& filename, bool flipY)
{
    if (filename.empty()) return nullptr;
    if (!ensureCOM()) return nullptr;

    IWICImagingFactory* factory = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (FAILED(hr) || !factory) return nullptr;

    IWICBitmapDecoder* decoder = nullptr;
    std::wstring wpath = toWide(filename);

    hr = factory->CreateDecoderFromFilename(
        wpath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);

    if (FAILED(hr) || !decoder)
    {
        factory->Release();
        return nullptr;
    }

    IWICBitmapFrameDecode* frame = nullptr;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr) || !frame)
    {
        decoder->Release();
        factory->Release();
        return nullptr;
    }

    UINT w = 0, h = 0;
    hr = frame->GetSize(&w, &h);
    if (FAILED(hr) || w == 0 || h == 0)
    {
        frame->Release();
        decoder->Release();
        factory->Release();
        return nullptr;
    }

    IWICFormatConverter* converter = nullptr;
    hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr) || !converter)
    {
        frame->Release();
        decoder->Release();
        factory->Release();
        return nullptr;
    }

    hr = converter->Initialize(
        frame,
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeCustom
    );

    if (FAILED(hr))
    {
        converter->Release();
        frame->Release();
        decoder->Release();
        factory->Release();
        return nullptr;
    }

    const UINT stride = w * 4u;
    const UINT bufSize = stride * h;

    std::vector<std::uint8_t> pixels(bufSize);
    hr = converter->CopyPixels(nullptr, stride, bufSize, pixels.data());

    converter->Release();
    frame->Release();
    decoder->Release();
    factory->Release();

    if (FAILED(hr)) return nullptr;

    if (flipY) flipRGBA(pixels, w, h);

    auto img = std::make_shared<Image>((int)w, (int)h, 4);
    img->data() = std::move(pixels); // lazy upload on bind
    return img;
}

} // namespace bonsai
