// examples/rotating_triangle_external_host.cpp
//
// External-context example (host owns Win32/WGL context) that renders a textured
// triangle and rotates it about its center at 10 degrees per second.
//
// Uses:
// - WinGLHostWindow (host window + context)
// - bonsai::Viewer in external mode
// - bonsai::Shader (ARB shader objects)
// - bonsai::Geometry with lazy Image upload/bind
//
// Build: link opengl32 + your bonsai library (and include TestUtils if you use it).

#include <memory>
#include <string>
#include <cstdint>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Viewer.h"
#include "Group.h"
#include "Geode.h"
#include "Geometry.h"
#include "Image.h"
#include "Shader.h"
#include "TestUtils.h"

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <GL/gl.h>
#endif

// ------------------------------
// Host window (owns WGL context)
// ------------------------------
class WinGLHostWindow
{
public:
    bool create(int w, int h, const char* title)
    {
        _w = w; _h = h;
        _hInstance = GetModuleHandleA(nullptr);

        WNDCLASSEXA wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = &WinGLHostWindow::WndProcStatic;
        wc.hInstance = _hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = "ose_external_host_rot_wndclass";
        RegisterClassExA(&wc);

        DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        RECT rc{0, 0, w, h};
        AdjustWindowRect(&rc, style, FALSE);

        _hWnd = CreateWindowExA(
            0, wc.lpszClassName, title, style,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rc.right - rc.left, rc.bottom - rc.top,
            nullptr, nullptr, _hInstance, this);

        if (!_hWnd) return false;

        _hDC = GetDC(_hWnd);
        if (!_hDC) return false;

        if (!setupPixelFormat())
            return false;

        _hGLRC = wglCreateContext(_hDC);
        if (!_hGLRC) return false;

        makeCurrent();

        ShowWindow(_hWnd, SW_SHOW);
        UpdateWindow(_hWnd);
        return true;
    }

    void destroy()
    {
        if (_hGLRC)
        {
            if (wglGetCurrentContext() == _hGLRC)
                wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(_hGLRC);
            _hGLRC = nullptr;
        }

        if (_hWnd && _hDC)
        {
            ReleaseDC(_hWnd, _hDC);
            _hDC = nullptr;
        }

        if (_hWnd)
        {
            DestroyWindow(_hWnd);
            _hWnd = nullptr;
        }
    }

    bool pump()
    {
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) return false;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        return !_shouldClose;
    }

    void makeCurrent() { wglMakeCurrent(_hDC, _hGLRC); }
    void swap() { SwapBuffers(_hDC); }

    int width() const { return _w; }
    int height() const { return _h; }

private:
    bool setupPixelFormat()
    {
        PIXELFORMATDESCRIPTOR pfd{};
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;
        pfd.cStencilBits = 8;
        pfd.iLayerType = PFD_MAIN_PLANE;

        int pf = ChoosePixelFormat(_hDC, &pfd);
        if (pf == 0) return false;
        if (!SetPixelFormat(_hDC, pf, &pfd)) return false;
        return true;
    }

    static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        WinGLHostWindow* self = nullptr;
        if (msg == WM_NCCREATE)
        {
            auto* cs = reinterpret_cast<CREATESTRUCTA*>(lp);
            self = reinterpret_cast<WinGLHostWindow*>(cs->lpCreateParams);
            SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)self);
        }
        else
        {
            self = reinterpret_cast<WinGLHostWindow*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
        }
        return self ? self->WndProc(hWnd, msg, wp, lp) : DefWindowProcA(hWnd, msg, wp, lp);
    }

    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        switch (msg)
        {
            case WM_CLOSE:
                _shouldClose = true;
                DestroyWindow(hWnd);
                return 0;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            case WM_SIZE:
                _w = (int)LOWORD(lp);
                _h = (int)HIWORD(lp);
                if (_w < 1) _w = 1;
                if (_h < 1) _h = 1;
                return 0;
            case WM_KEYDOWN:
                if (wp == VK_ESCAPE)
                {
                    _shouldClose = true;
                    DestroyWindow(hWnd);
                    return 0;
                }
                break;
            default:
                break;
        }
        return DefWindowProcA(hWnd, msg, wp, lp);
    }

private:
    HINSTANCE _hInstance{nullptr};
    HWND _hWnd{nullptr};
    HDC _hDC{nullptr};
    HGLRC _hGLRC{nullptr};

    int _w{0}, _h{0};
    bool _shouldClose{false};
};

// ------------------------------
// Main
// ------------------------------
int main()
{
    using clock_t = std::chrono::steady_clock;

    // Host creates window + GL context
    WinGLHostWindow host;
    if (!host.create(800, 600, "Rotating Triangle (10 deg/sec) - External Context"))
        return 1;

    // Viewer uses external mode (host owns context and swap)
    bonsai::Viewer viewer;
    viewer.openExternal(
        host.width(),
        host.height(),
        [&]() { host.makeCurrent(); },
        [&]() { host.swap(); });

    // Must have current context for shader compilation
    host.makeCurrent();

    // Shader: uses built-in attributes (gl_Vertex, gl_MultiTexCoord0)
    const std::string vs = R"GLSL(
        uniform mat4 uMVP;
        varying vec2 vUV;
        void main()
        {
            vUV = gl_MultiTexCoord0.xy;
            gl_Position = uMVP * gl_Vertex;
        }
    )GLSL";

    const std::string fs = R"GLSL(
        uniform sampler2D uTexture0;
        varying vec2 vUV;
        void main()
        {
            gl_FragColor = texture2D(uTexture0, vUV);
        }
    )GLSL";

    auto shader = std::make_shared<bonsai::Shader>();
    if (!shader->buildFromSource(vs, fs))
        return 2;

    viewer.setShader(shader);

    // Build scene graph
    auto root  = std::make_shared<bonsai::Group>();
    auto geode = std::make_shared<bonsai::Geode>();
    root->addChild(geode);

    auto geom = std::make_shared<bonsai::Geometry>();
    geom->setPrimitive(bonsai::Geometry::Primitive::Triangles);

    // Triangle centered at origin (so rotation about center is simple)
    geom->vertices() = {
        {-0.6f, -0.5f, 0.0f},
        { 0.6f, -0.5f, 0.0f},
        { 0.0f,  0.7f, 0.0f},
    };

    geom->texcoords() = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.5f, 1.0f},
    };

    const std::uint8_t a[4] = {255, 255, 255, 255};
    const std::uint8_t b[4] = { 40, 160, 255, 255};
    auto tex = bonsai::test::TestUtils::makeCheckerWxH_RGBA(256, 256, a, b);
    geom->setTexture(0, tex);

    geode->addDrawable(geom);
    viewer.setSceneData(root);

    // Camera
    glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 2.0f),
                                 glm::vec3(0, 0, 0),
                                 glm::vec3(0, 1, 0));
    viewer.setViewMatrix(view);

    // Timing for rotation: 10 degrees per second
    const float degPerSec = 10.0f;
    auto t0 = clock_t::now();

    while (host.pump())
    {
        host.makeCurrent();

        // Keep viewer size in sync + update projection for aspect changes
        viewer.setExternalSize(host.width(), host.height());

        glm::mat4 proj = glm::perspective(glm::radians(60.0f),
                                          (float)host.width() / (float)host.height(),
                                          0.1f, 10.0f);
        viewer.setProjectionMatrix(proj);

        // Compute elapsed seconds
        auto t1 = clock_t::now();
        std::chrono::duration<float> dt = t1 - t0;
        float seconds = dt.count();

        // Angle in radians
        float angleRad = glm::radians(degPerSec * seconds);

        // Rotate the Geode about Z axis around origin
        geode->setTransform(glm::rotate(glm::mat4(1.0f), angleRad, glm::vec3(0, 0, 1)));

        viewer.renderOnce();
    }

    viewer.close();
    host.destroy();
    return 0;
}