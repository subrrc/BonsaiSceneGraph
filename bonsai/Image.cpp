#include "Image.h"

#if defined(_WIN32)
#include <windows.h>
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace bonsai {

    // OpenGL enums might not be present in older headers on Windows
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif

// Function pointer storage
    bool Image::_activeTexLoaded = false;
    void (*Image::_glActiveTexture)(unsigned int texture) = nullptr;

    void Image::ensureActiveTextureLoaded()
    {
        if (_activeTexLoaded) return;
        _activeTexLoaded = true;

#if defined(_WIN32)
        // glActiveTexture is core since 1.3 but on Windows it usually must be fetched.
        _glActiveTexture = (void(*)(unsigned int))wglGetProcAddress("glActiveTexture");
#else
        // On non-Windows you could use glXGetProcAddress; omitted for simplicity
        _glActiveTexture = nullptr;
#endif
    }

    void Image::activeTexture(int unit)
    {
        ensureActiveTextureLoaded();
        if (_glActiveTexture)
        {
            _glActiveTexture((unsigned int)(GL_TEXTURE0 + unit));
        }
        else
        {
            // No multitexture support exposed; silently fall back to unit 0.
            // (Your project uses one texture at a time, so this is fine.)
            (void)unit;
        }
    }

    Image::Image(int w, int h, int channels)
        : _w(w), _h(h), _channels(channels)
    {
        _data.resize(static_cast<size_t>(_w) * static_cast<size_t>(_h) * static_cast<size_t>(_channels), 255);
        _dirty = true;
    }

    void Image::setData(int w, int h, int channels, const std::vector<std::uint8_t>& pixels)
    {
        _w = w;
        _h = h;
        _channels = channels;
        _data = pixels;
        _dirty = true;
    }

    void Image::ensureUploaded()
    {
        if (!_dirty) return;

        if (_w <= 0 || _h <= 0 || _channels <= 0 || _data.empty())
            return;

        if (_texId == 0)
            glGenTextures(1, &_texId);

        glBindTexture(GL_TEXTURE_2D, _texId);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        GLenum format = (_channels == 4) ? GL_RGBA : (_channels == 3 ? GL_RGB : GL_LUMINANCE);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            (format == GL_RGBA ? GL_RGBA : (format == GL_RGB ? GL_RGB : GL_LUMINANCE)),
            _w,
            _h,
            0,
            format,
            GL_UNSIGNED_BYTE,
            _data.data()
        );

        _dirty = false;
    }

    void Image::uploadToGL()
    {
        ensureUploaded();
    }

    void Image::bind(int textureUnit)
    {
        // Best effort multitexture selection; falls back to unit 0 if unavailable.
        activeTexture(textureUnit);

        ensureUploaded();
        glBindTexture(GL_TEXTURE_2D, _texId);
    }

    void Image::destroyGL()
    {
        if (_texId != 0)
        {
            glDeleteTextures(1, &_texId);
            _texId = 0;
        }
        _dirty = true;
    }

} // namespace bonsai