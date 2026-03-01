#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>

#include "Node.h"
#include "RenderState.h"
#include "Shader.h"

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#endif

namespace bonsai {

class Viewer
{
public:
    using MakeCurrentFn = std::function<void()>;
    using SwapBuffersFn = std::function<void()>;

    Viewer();
    ~Viewer();

    void setSceneData(const NodePtr& root) { _root = root; }
    void setClearColor(const glm::vec4& c) { _clear = c; }

    void setViewMatrix(const glm::mat4& v) { _state.view = v; }
    void setProjectionMatrix(const glm::mat4& p) { _state.proj = p; }

    // Global shader (optional)
    void setShader(const std::shared_ptr<Shader>& s)
    {
        _shader = s;
        _state.shader = _shader.get();
    }

    // -----------------------
    // Internal window (WGL)
    // -----------------------
    bool open(int width, int height, const char* title);
    int  run(int width, int height, const char* title);

    // NEW: one-frame step for windowed mode (poll events + render). Great for animation loops.
    // Returns false if the viewer is closing/closed.
    bool frame();

    // -----------------------
    // External context mode
    // -----------------------
    // Use when a host framework owns the GL context and swap chain.
    bool openExternal(int width, int height, MakeCurrentFn makeCurrent, SwapBuffersFn swapBuffers);

    // Host should call this on resize (or whenever size changes).
    void setExternalSize(int width, int height);

    // -----------------------
    // Offscreen (hidden WGL)
    // -----------------------
    bool openOffscreen(int width, int height);

    // Common
    void close();
    bool isOpen() const;

    // Render a single frame (internal window, external context, or offscreen)
    void renderOnce();

    // Read back last rendered color buffer as RGBA8
    std::vector<std::uint8_t> getColorBufferRGBA8(bool flipY = true) const;

    // If offscreen uses an FBO, returns its color texture id; else 0.
    unsigned int getColorTextureId() const;

    // Save PNG
    bool savePNG(const char* filename, bool flipY = true);

    int width() const;
    int height() const;

private:
#if defined(_WIN32)
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

    enum class Mode
    {
        None,
        InternalWindow,
        Offscreen,
        External
    };

    struct Window;
    struct Offscreen;

    // Internal WGL window creation
    bool createWindow(int w, int h, const char* title, bool visible);
    void destroyWindow();
    bool pollEvents(); // internal window loop only

    // Context handling
    void makeCurrent();
    void present();

    // Core render
    void renderScene();

    // Offscreen target
    bool initOffscreenTarget(int w, int h);
    void destroyOffscreenTarget();
    void bindOffscreenTarget() const;
    void unbindOffscreenTarget() const;

    // PNG writing helper
    static bool writePNG_RGBA8(const char* filename, int w, int h, const std::uint8_t* rgbaTopDown);

private:
    Mode _mode{Mode::None};

    // Internal window resources
    std::unique_ptr<Window> _window;

    // Offscreen target
    std::unique_ptr<Offscreen> _offscreen;

    // External callbacks + size
    MakeCurrentFn _externalMakeCurrent;
    SwapBuffersFn _externalSwapBuffers;
    int _externalW{0};
    int _externalH{0};

    NodePtr _root;
    RenderState _state;

    glm::vec4 _clear{0.1f, 0.1f, 0.12f, 1.0f};

    std::shared_ptr<Shader> _shader;
};

} // namespace bonsai