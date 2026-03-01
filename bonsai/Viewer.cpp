#include "Viewer.h"

#if defined(_WIN32)
  #include <GL/gl.h>
#else
  #error "This Viewer.cpp implementation is Windows-only."
#endif

#include <cstdio>
#include <cstring>
#include <algorithm>

namespace bonsai {

// ----------------- FBO EXT loader -----------------
using PFNGLGENFRAMEBUFFERSEXTPROC         = void (APIENTRY*)(GLsizei, GLuint*);
using PFNGLBINDFRAMEBUFFEREXTPROC         = void (APIENTRY*)(GLenum, GLuint);
using PFNGLDELETEFRAMEBUFFERSEXTPROC      = void (APIENTRY*)(GLsizei, const GLuint*);
using PFNGLFRAMEBUFFERTEXTURE2DEXTPROC    = void (APIENTRY*)(GLenum, GLenum, GLenum, GLuint, GLint);
using PFNGLGENRENDERBUFFERSEXTPROC        = void (APIENTRY*)(GLsizei, GLuint*);
using PFNGLBINDRENDERBUFFEREXTPROC        = void (APIENTRY*)(GLenum, GLuint);
using PFNGLDELETERENDERBUFFERSEXTPROC     = void (APIENTRY*)(GLsizei, const GLuint*);
using PFNGLRENDERBUFFERSTORAGEEXTPROC     = void (APIENTRY*)(GLenum, GLenum, GLsizei, GLsizei);
using PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC = void (APIENTRY*)(GLenum, GLenum, GLenum, GLuint);
using PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC  = GLenum (APIENTRY*)(GLenum);

static PFNGLGENFRAMEBUFFERSEXTPROC          glGenFramebuffersEXT_          = nullptr;
static PFNGLBINDFRAMEBUFFEREXTPROC          glBindFramebufferEXT_          = nullptr;
static PFNGLDELETEFRAMEBUFFERSEXTPROC       glDeleteFramebuffersEXT_       = nullptr;
static PFNGLFRAMEBUFFERTEXTURE2DEXTPROC     glFramebufferTexture2DEXT_     = nullptr;
static PFNGLGENRENDERBUFFERSEXTPROC         glGenRenderbuffersEXT_         = nullptr;
static PFNGLBINDRENDERBUFFEREXTPROC         glBindRenderbufferEXT_         = nullptr;
static PFNGLDELETERENDERBUFFERSEXTPROC      glDeleteRenderbuffersEXT_      = nullptr;
static PFNGLRENDERBUFFERSTORAGEEXTPROC      glRenderbufferStorageEXT_      = nullptr;
static PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC  glFramebufferRenderbufferEXT_  = nullptr;
static PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC   glCheckFramebufferStatusEXT_   = nullptr;

// Header-safe fallback enums (Windows gl.h is old)
#ifndef GL_FRAMEBUFFER_EXT
  #define GL_FRAMEBUFFER_EXT 0x8D40
#endif
#ifndef GL_RENDERBUFFER_EXT
  #define GL_RENDERBUFFER_EXT 0x8D41
#endif
#ifndef GL_COLOR_ATTACHMENT0_EXT
  #define GL_COLOR_ATTACHMENT0_EXT 0x8CE0
#endif
#ifndef GL_DEPTH_ATTACHMENT_EXT
  #define GL_DEPTH_ATTACHMENT_EXT 0x8D00
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE_EXT
  #define GL_FRAMEBUFFER_COMPLETE_EXT 0x8CD5
#endif
#ifndef GL_DEPTH_COMPONENT24
  #define GL_DEPTH_COMPONENT24 0x81A6
#endif
#ifndef GL_RGBA8
  #define GL_RGBA8 0x8058
#endif

static bool hasExtension(const char* extList, const char* ext)
{
    if (!extList || !ext) return false;
    const size_t extLen = std::strlen(ext);
    const char* start = extList;

    while (true)
    {
        const char* pos = std::strstr(start, ext);
        if (!pos) return false;
        const char before = (pos == extList) ? ' ' : *(pos - 1);
        const char after  = *(pos + extLen);
        if ((before == ' ' || before == '\0') && (after == ' ' || after == '\0'))
            return true;
        start = pos + extLen;
    }
}

static bool loadFBOEXT()
{
    static bool attempted = false;
    static bool ok = false;
    if (attempted) return ok;
    attempted = true;

    const char* ext = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (!hasExtension(ext, "GL_EXT_framebuffer_object"))
    {
        ok = false;
        return ok;
    }

    glGenFramebuffersEXT_          = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
    glBindFramebufferEXT_          = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
    glDeleteFramebuffersEXT_       = (PFNGLDELETEFRAMEBUFFERSEXTPROC)wglGetProcAddress("glDeleteFramebuffersEXT");
    glFramebufferTexture2DEXT_     = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
    glGenRenderbuffersEXT_         = (PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT");
    glBindRenderbufferEXT_         = (PFNGLBINDRENDERBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT");
    glDeleteRenderbuffersEXT_      = (PFNGLDELETERENDERBUFFERSEXTPROC)wglGetProcAddress("glDeleteRenderbuffersEXT");
    glRenderbufferStorageEXT_      = (PFNGLRENDERBUFFERSTORAGEEXTPROC)wglGetProcAddress("glRenderbufferStorageEXT");
    glFramebufferRenderbufferEXT_  = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)wglGetProcAddress("glFramebufferRenderbufferEXT");
    glCheckFramebufferStatusEXT_   = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");

    ok = glGenFramebuffersEXT_ && glBindFramebufferEXT_ && glDeleteFramebuffersEXT_ &&
         glFramebufferTexture2DEXT_ && glGenRenderbuffersEXT_ && glBindRenderbufferEXT_ &&
         glDeleteRenderbuffersEXT_ && glRenderbufferStorageEXT_ && glFramebufferRenderbufferEXT_ &&
         glCheckFramebufferStatusEXT_;

    return ok;
}

static void loadMatrixGL(const glm::mat4& m) { glLoadMatrixf(&m[0][0]); }

// ---------------- Window ----------------
struct Viewer::Window
{
    HINSTANCE hInstance{nullptr};
    HWND      hWnd{nullptr};
    HDC       hDC{nullptr};
    HGLRC     hGLRC{nullptr};

    int  w{0}, h{0};
    bool shouldClose{false};

    ATOM wndClassAtom{0};
};

// ---------------- Offscreen target ----------------
struct Viewer::Offscreen
{
    int w{0}, h{0};

    bool useFBO{false};
    GLuint fbo{0};
    GLuint colorTex{0};
    GLuint depthRb{0};
};

Viewer::Viewer() = default;

Viewer::~Viewer()
{
    close();
}

static bool SetupPixelFormat(HDC hDC)
{
    PIXELFORMATDESCRIPTOR pfd;
    std::memset(&pfd, 0, sizeof(pfd));

    pfd.nSize        = sizeof(pfd);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType   = PFD_TYPE_RGBA;
    pfd.cColorBits   = 32;
    pfd.cDepthBits   = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType   = PFD_MAIN_PLANE;

    int pf = ChoosePixelFormat(hDC, &pfd);
    if (pf == 0) return false;
    if (!SetPixelFormat(hDC, pf, &pfd)) return false;
    return true;
}

// ---------------- Public open modes ----------------

bool Viewer::open(int width, int height, const char* title)
{
    if (isOpen()) return true;

    _mode = Mode::InternalWindow;
    _externalMakeCurrent = {};
    _externalSwapBuffers = {};
    _externalW = _externalH = 0;

    if (!createWindow(width, height, title, /*visible*/true))
    {
        _mode = Mode::None;
        return false;
    }

    makeCurrent();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);

    _state.shader = _shader.get();
    return true;
}

bool Viewer::openOffscreen(int width, int height)
{
    if (isOpen()) return true;

    _mode = Mode::Offscreen;
    _externalMakeCurrent = {};
    _externalSwapBuffers = {};
    _externalW = _externalH = 0;

    if (!createWindow(width, height, "ose_offscreen", /*visible*/false))
    {
        _mode = Mode::None;
        return false;
    }

    makeCurrent();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);

    if (!initOffscreenTarget(width, height))
        std::printf("[bonsai::Viewer] Offscreen target init failed; will read from backbuffer.\n");

    _state.shader = _shader.get();
    return true;
}

bool Viewer::openExternal(int width, int height, MakeCurrentFn makeCurrentFn, SwapBuffersFn swapBuffersFn)
{
    close(); // release internal/offscreen resources

    _mode = Mode::External;
    _externalMakeCurrent = std::move(makeCurrentFn);
    _externalSwapBuffers = std::move(swapBuffersFn);
    _externalW = width;
    _externalH = height;

    _state.shader = _shader.get();
    return true;
}

void Viewer::setExternalSize(int width, int height)
{
    _externalW = width;
    _externalH = height;
}

void Viewer::close()
{
    destroyOffscreenTarget();
    destroyWindow();

    _externalMakeCurrent = {};
    _externalSwapBuffers = {};
    _externalW = _externalH = 0;

    _mode = Mode::None;
}

bool Viewer::isOpen() const
{
    if (_mode == Mode::External)
        return (bool)_externalMakeCurrent;

    return _window && _window->hWnd && _window->hDC && _window->hGLRC && !_window->shouldClose;
}

int Viewer::width() const
{
    if (_mode == Mode::External) return _externalW;
    return _window ? _window->w : 0;
}

int Viewer::height() const
{
    if (_mode == Mode::External) return _externalH;
    return _window ? _window->h : 0;
}

// ---------------- Internal window implementation ----------------

bool Viewer::createWindow(int w, int h, const char* title, bool visible)
{
    destroyWindow();

    _window = std::make_unique<Window>();
    _window->w = w;
    _window->h = h;
    _window->hInstance = GetModuleHandleA(nullptr);

    const char* className = "ose_viewer_wndclass";

    WNDCLASSEXA wc;
    std::memset(&wc, 0, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = Viewer::WndProc;
    wc.hInstance     = _window->hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = className;

    _window->wndClassAtom = RegisterClassExA(&wc);
    if (_window->wndClassAtom == 0)
    {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS)
        {
            std::printf("[bonsai::Viewer] RegisterClassExA failed (%lu)\n", (unsigned long)err);
            _window.reset();
            return false;
        }
    }

    DWORD style = visible ? (WS_OVERLAPPEDWINDOW | WS_VISIBLE) : WS_POPUP;
    RECT rc{0, 0, w, h};
    AdjustWindowRect(&rc, style, FALSE);
    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;

    _window->hWnd = CreateWindowExA(
        0,
        className,
        title ? title : "BonsaiSceneGraph viewer",
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        winW,
        winH,
        nullptr,
        nullptr,
        _window->hInstance,
        this
    );

    if (!_window->hWnd)
    {
        std::printf("[bonsai::Viewer] CreateWindowExA failed (%lu)\n", (unsigned long)GetLastError());
        destroyWindow();
        return false;
    }

    if (!visible)
    {
        ShowWindow(_window->hWnd, SW_HIDE);
        UpdateWindow(_window->hWnd);
    }

    _window->hDC = GetDC(_window->hWnd);
    if (!_window->hDC)
    {
        std::printf("[bonsai::Viewer] GetDC failed\n");
        destroyWindow();
        return false;
    }

    if (!SetupPixelFormat(_window->hDC))
    {
        std::printf("[bonsai::Viewer] SetupPixelFormat failed\n");
        destroyWindow();
        return false;
    }

    _window->hGLRC = wglCreateContext(_window->hDC);
    if (!_window->hGLRC)
    {
        std::printf("[bonsai::Viewer] wglCreateContext failed (%lu)\n", (unsigned long)GetLastError());
        destroyWindow();
        return false;
    }

    if (!wglMakeCurrent(_window->hDC, _window->hGLRC))
    {
        std::printf("[bonsai::Viewer] wglMakeCurrent failed (%lu)\n", (unsigned long)GetLastError());
        destroyWindow();
        return false;
    }

    if (visible)
    {
        ShowWindow(_window->hWnd, SW_SHOW);
        UpdateWindow(_window->hWnd);
    }

    return true;
}

void Viewer::destroyWindow()
{
    if (!_window) return;

    if (wglGetCurrentContext() == _window->hGLRC)
        wglMakeCurrent(nullptr, nullptr);

    if (_window->hGLRC)
    {
        wglDeleteContext(_window->hGLRC);
        _window->hGLRC = nullptr;
    }

    if (_window->hWnd && _window->hDC)
    {
        ReleaseDC(_window->hWnd, _window->hDC);
        _window->hDC = nullptr;
    }

    if (_window->hWnd)
    {
        DestroyWindow(_window->hWnd);
        _window->hWnd = nullptr;
    }

    _window.reset();
}

bool Viewer::pollEvents()
{
    if (!_window) return false;

    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            _window->shouldClose = true;
            return false;
        }

        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return !_window->shouldClose;
}

// ---------------- Context + present ----------------

void Viewer::makeCurrent()
{
    if (_mode == Mode::External)
    {
        if (_externalMakeCurrent) _externalMakeCurrent();
        return;
    }

    if (!_window || !_window->hDC || !_window->hGLRC) return;
    wglMakeCurrent(_window->hDC, _window->hGLRC);
}

void Viewer::present()
{
    if (_mode == Mode::External)
    {
        if (_externalSwapBuffers) _externalSwapBuffers();
        return;
    }

    if (!_window || !_window->hDC) return;
    SwapBuffers(_window->hDC);
}

// ---------------- Offscreen target ----------------

bool Viewer::initOffscreenTarget(int w, int h)
{
    destroyOffscreenTarget();
    _offscreen = std::make_unique<Offscreen>();
    _offscreen->w = w;
    _offscreen->h = h;

    if (!loadFBOEXT())
    {
        _offscreen->useFBO = false;
        return false;
    }

    glGenTextures(1, &_offscreen->colorTex);
    glBindTexture(GL_TEXTURE_2D, _offscreen->colorTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glGenRenderbuffersEXT_(1, &_offscreen->depthRb);
    glBindRenderbufferEXT_(GL_RENDERBUFFER_EXT, _offscreen->depthRb);
    glRenderbufferStorageEXT_(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, w, h);

    glGenFramebuffersEXT_(1, &_offscreen->fbo);
    glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, _offscreen->fbo);

    glFramebufferTexture2DEXT_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, _offscreen->colorTex, 0);
    glFramebufferRenderbufferEXT_(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, _offscreen->depthRb);

    GLenum status = glCheckFramebufferStatusEXT_(GL_FRAMEBUFFER_EXT);
    glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
    {
        std::printf("[bonsai::Viewer] FBO incomplete (0x%X)\n", (unsigned)status);
        destroyOffscreenTarget();
        return false;
    }

    _offscreen->useFBO = true;
    return true;
}

void Viewer::destroyOffscreenTarget()
{
    if (!_offscreen) return;

    if (_offscreen->useFBO && loadFBOEXT())
    {
        if (_offscreen->fbo)
        {
            glDeleteFramebuffersEXT_(1, &_offscreen->fbo);
            _offscreen->fbo = 0;
        }
        if (_offscreen->depthRb)
        {
            glDeleteRenderbuffersEXT_(1, &_offscreen->depthRb);
            _offscreen->depthRb = 0;
        }
    }

    if (_offscreen->colorTex)
    {
        glDeleteTextures(1, &_offscreen->colorTex);
        _offscreen->colorTex = 0;
    }

    _offscreen.reset();
}

void Viewer::bindOffscreenTarget() const
{
    if (!_offscreen || !_offscreen->useFBO || !loadFBOEXT())
        return;
    glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, _offscreen->fbo);
}

void Viewer::unbindOffscreenTarget() const
{
    if (!_offscreen || !_offscreen->useFBO || !loadFBOEXT())
        return;
    glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, 0);
}

// ---------------- Rendering ----------------

void Viewer::renderScene()
{
    const int w = (_mode == Mode::Offscreen && _offscreen) ? _offscreen->w :
                  (_mode == Mode::External) ? _externalW :
                  (_window ? _window->w : 0);

    const int h = (_mode == Mode::Offscreen && _offscreen) ? _offscreen->h :
                  (_mode == Mode::External) ? _externalH :
                  (_window ? _window->h : 0);

    if (w <= 0 || h <= 0)
        return;

    if (_mode == Mode::Offscreen && _offscreen && _offscreen->useFBO)
        bindOffscreenTarget();

    glViewport(0, 0, w, h);
    glClearColor(_clear.r, _clear.g, _clear.b, _clear.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    loadMatrixGL(_state.proj);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    _state.model = glm::mat4(1.0f);
    _state.shader = _shader.get();

    if (_root)
        _root->traverse(_state);

    if (_mode == Mode::Offscreen && _offscreen && _offscreen->useFBO)
        unbindOffscreenTarget();
}

void Viewer::renderOnce()
{
    makeCurrent();
    renderScene();
    present();
}

// NEW: per-frame tick for windowed mode
bool Viewer::frame()
{
    if (_mode == Mode::External)
    {
        // Host owns event loop; we can still render one frame.
        renderOnce();
        return true;
    }

    if (_mode == Mode::Offscreen)
    {
        if (!isOpen()) return false;
        renderOnce();
        return true;
    }

    // Internal window
    if (_mode != Mode::InternalWindow) return false;
    if (!isOpen()) return false;

    if (!pollEvents())
    {
        close();
        return false;
    }

    renderOnce();
    return true;
}

// ---------------- Readback / PNG ----------------

std::vector<std::uint8_t> Viewer::getColorBufferRGBA8(bool flipY) const
{
    std::vector<std::uint8_t> out;

    const int w = (_mode == Mode::Offscreen && _offscreen) ? _offscreen->w :
                  (_mode == Mode::External) ? _externalW :
                  (_window ? _window->w : 0);

    const int h = (_mode == Mode::Offscreen && _offscreen) ? _offscreen->h :
                  (_mode == Mode::External) ? _externalH :
                  (_window ? _window->h : 0);

    if (w <= 0 || h <= 0) return out;

    out.resize((size_t)w * (size_t)h * 4u);

    if (_mode == Mode::Offscreen && _offscreen && _offscreen->useFBO && loadFBOEXT())
    {
        glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, _offscreen->fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    }
    else
    {
        glReadBuffer(GL_BACK);
    }

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, out.data());

    if (_mode == Mode::Offscreen && _offscreen && _offscreen->useFBO && loadFBOEXT())
        glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, 0);

    if (!flipY)
        return out;

    const size_t rowBytes = (size_t)w * 4u;
    std::vector<std::uint8_t> tmp(rowBytes);

    for (int y = 0; y < h / 2; ++y)
    {
        std::uint8_t* rowA = out.data() + (size_t)y * rowBytes;
        std::uint8_t* rowB = out.data() + (size_t)(h - 1 - y) * rowBytes;

        std::memcpy(tmp.data(), rowA, rowBytes);
        std::memcpy(rowA, rowB, rowBytes);
        std::memcpy(rowB, tmp.data(), rowBytes);
    }

    return out;
}

unsigned int Viewer::getColorTextureId() const
{
    if (_mode != Mode::Offscreen || !_offscreen || !_offscreen->useFBO)
        return 0;
    return (unsigned int)_offscreen->colorTex;
}

bool Viewer::savePNG(const char* filename, bool flipY)
{
    if (!filename || !*filename) return false;

    makeCurrent();

    const int w = width();
    const int h = height();
    if (w <= 0 || h <= 0) return false;

    std::vector<std::uint8_t> rgbaTopDown = getColorBufferRGBA8(flipY);
    if (rgbaTopDown.empty()) return false;

    return writePNG_RGBA8(filename, w, h, rgbaTopDown.data());
}

// ---------------- Internal run loop ----------------

int Viewer::run(int width, int height, const char* title)
{
    if (_mode == Mode::External)
    {
        // Host owns the loop; we provide renderOnce()/frame()
        (void)width; (void)height; (void)title;
        renderOnce();
        return 0;
    }

    if (!isOpen())
    {
        if (!open(width, height, title))
            return 1;
    }

    while (frame())
        ; // frame() polls events + renders

    return 0;
}

// ---------------- Win32 WndProc ----------------

LRESULT CALLBACK Viewer::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCTA*>(lParam);
        auto* viewer = reinterpret_cast<Viewer*>(cs->lpCreateParams);
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(viewer));
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }

    auto* viewer = reinterpret_cast<Viewer*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));

    switch (msg)
    {
        case WM_CLOSE:
        {
            if (viewer && viewer->_window)
                viewer->_window->shouldClose = true;
            DestroyWindow(hWnd);
            return 0;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        case WM_SIZE:
        {
            if (viewer && viewer->_window)
            {
                int w = LOWORD(lParam);
                int h = HIWORD(lParam);
                viewer->_window->w = (w > 0) ? w : 1;
                viewer->_window->h = (h > 0) ? h : 1;
            }
            return 0;
        }
        case WM_KEYDOWN:
        {
            if (wParam == VK_ESCAPE)
            {
                if (viewer && viewer->_window)
                    viewer->_window->shouldClose = true;
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }
        default:
            break;
    }

    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

// ---------------- PNG writer ----------------

static void write_u32_be(std::FILE* f, std::uint32_t v)
{
    std::uint8_t b[4] = {
        (std::uint8_t)((v >> 24) & 0xFF),
        (std::uint8_t)((v >> 16) & 0xFF),
        (std::uint8_t)((v >> 8) & 0xFF),
        (std::uint8_t)(v & 0xFF)
    };
    std::fwrite(b, 1, 4, f);
}

static std::uint32_t crc32_update(std::uint32_t crc, const std::uint8_t* data, size_t len)
{
    crc = ~crc;
    for (size_t i = 0; i < len; ++i)
    {
        crc ^= data[i];
        for (int k = 0; k < 8; ++k)
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320u : (crc >> 1);
    }
    return ~crc;
}

static std::uint32_t adler32(const std::uint8_t* data, size_t len)
{
    const std::uint32_t MOD = 65521u;
    std::uint32_t a = 1, b = 0;
    for (size_t i = 0; i < len; ++i)
    {
        a = (a + data[i]) % MOD;
        b = (b + a) % MOD;
    }
    return (b << 16) | a;
}

static void write_chunk(std::FILE* f, const char type[4], const std::uint8_t* data, std::uint32_t len)
{
    write_u32_be(f, len);
    std::fwrite(type, 1, 4, f);
    if (len && data) std::fwrite(data, 1, len, f);

    std::uint32_t crc = 0;
    crc = crc32_update(crc, reinterpret_cast<const std::uint8_t*>(type), 4);
    if (len && data) crc = crc32_update(crc, data, len);
    write_u32_be(f, crc);
}

bool Viewer::writePNG_RGBA8(const char* filename, int w, int h, const std::uint8_t* rgbaTopDown)
{
    if (w <= 0 || h <= 0 || !rgbaTopDown) return false;

    std::FILE* f = std::fopen(filename, "wb");
    if (!f) return false;

    const std::uint8_t sig[8] = { 137,80,78,71,13,10,26,10 };
    std::fwrite(sig, 1, 8, f);

    std::uint8_t ihdr[13];
    ihdr[0] = (std::uint8_t)((w >> 24) & 0xFF);
    ihdr[1] = (std::uint8_t)((w >> 16) & 0xFF);
    ihdr[2] = (std::uint8_t)((w >> 8) & 0xFF);
    ihdr[3] = (std::uint8_t)(w & 0xFF);
    ihdr[4] = (std::uint8_t)((h >> 24) & 0xFF);
    ihdr[5] = (std::uint8_t)((h >> 16) & 0xFF);
    ihdr[6] = (std::uint8_t)((h >> 8) & 0xFF);
    ihdr[7] = (std::uint8_t)(h & 0xFF);
    ihdr[8]  = 8;
    ihdr[9]  = 6;
    ihdr[10] = 0;
    ihdr[11] = 0;
    ihdr[12] = 0;
    write_chunk(f, "IHDR", ihdr, 13);

    const size_t rowBytes = (size_t)w * 4u;
    const size_t rawSize = (rowBytes + 1u) * (size_t)h;

    std::vector<std::uint8_t> raw(rawSize);
    for (int y = 0; y < h; ++y)
    {
        const size_t dst = (size_t)y * (rowBytes + 1u);
        raw[dst] = 0;
        std::memcpy(&raw[dst + 1u], rgbaTopDown + (size_t)y * rowBytes, rowBytes);
    }

    std::vector<std::uint8_t> z;
    z.reserve(rawSize + 6 + rawSize / 65535 + 32);

    z.push_back(0x78);
    z.push_back(0x01);

    size_t remaining = raw.size();
    size_t offset = 0;

    while (remaining > 0)
    {
        const std::uint16_t blockLen = (std::uint16_t)std::min<size_t>(remaining, 65535);
        const bool final = (remaining <= 65535);

        z.push_back(final ? 0x01 : 0x00);

        z.push_back((std::uint8_t)(blockLen & 0xFF));
        z.push_back((std::uint8_t)((blockLen >> 8) & 0xFF));
        const std::uint16_t nlen = (std::uint16_t)(~blockLen);
        z.push_back((std::uint8_t)(nlen & 0xFF));
        z.push_back((std::uint8_t)((nlen >> 8) & 0xFF));

        z.insert(z.end(), raw.begin() + (ptrdiff_t)offset, raw.begin() + (ptrdiff_t)(offset + blockLen));

        offset += blockLen;
        remaining -= blockLen;
    }

    const std::uint32_t ad = adler32(raw.data(), raw.size());
    z.push_back((std::uint8_t)((ad >> 24) & 0xFF));
    z.push_back((std::uint8_t)((ad >> 16) & 0xFF));
    z.push_back((std::uint8_t)((ad >> 8) & 0xFF));
    z.push_back((std::uint8_t)(ad & 0xFF));

    write_chunk(f, "IDAT", z.data(), (std::uint32_t)z.size());
    write_chunk(f, "IEND", nullptr, 0);

    std::fclose(f);
    return true;
}

} // namespace bonsai