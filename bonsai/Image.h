#pragma once
#include <cstdint>
#include <vector>

namespace bonsai {

// Minimal image for texturing (RGBA8 recommended).
class Image
{
public:
    Image() = default;
    Image(int w, int h, int channels);

    int width() const { return _w; }
    int height() const { return _h; }
    int channels() const { return _channels; }

    // Access to CPU pixel data.
    // Mutating data marks the GPU texture as dirty.
    const std::vector<std::uint8_t>& data() const { return _data; }
    std::vector<std::uint8_t>& data() { _dirty = true; return _data; }

    void setData(int w, int h, int channels, const std::vector<std::uint8_t>& pixels);
    void markDirty() { _dirty = true; }

    // Force upload (requires current GL context)
    void uploadToGL();

    // Binds as GL_TEXTURE_2D (auto-uploads if needed).
    // textureUnit is best-effort: if glActiveTexture isn't available,
    // only unit 0 is used.
    void bind(int textureUnit = 0);

    void destroyGL();

    unsigned int textureId() const { return _texId; }
    bool isDirty() const { return _dirty; }

private:
    int _w{0}, _h{0}, _channels{0};
    std::vector<std::uint8_t> _data;

    unsigned int _texId{0};
    bool _dirty{true};

    void ensureUploaded();

    // NEW: lightweight loader for glActiveTexture (no external GL loader needed)
    static void ensureActiveTextureLoaded();
    static void activeTexture(int unit);

private:
    static bool _activeTexLoaded;
    static void (*_glActiveTexture)(unsigned int texture);
};

} // namespace bonsai