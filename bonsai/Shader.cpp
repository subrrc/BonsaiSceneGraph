#include "Shader.h"
#include "RenderState.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

// ---------- ARB shader objects typedefs ----------
#ifndef GLhandleARB
typedef unsigned int GLhandleARB;
#endif

#ifndef GLcharARB
typedef char GLcharARB;
#endif

#ifndef GL_OBJECT_COMPILE_STATUS_ARB
#define GL_OBJECT_COMPILE_STATUS_ARB 0x8B81
#endif
#ifndef GL_OBJECT_LINK_STATUS_ARB
#define GL_OBJECT_LINK_STATUS_ARB 0x8B82
#endif
#ifndef GL_OBJECT_INFO_LOG_LENGTH_ARB
#define GL_OBJECT_INFO_LOG_LENGTH_ARB 0x8B84
#endif

#ifndef GL_VERTEX_SHADER_ARB
#define GL_VERTEX_SHADER_ARB 0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER_ARB
#define GL_FRAGMENT_SHADER_ARB 0x8B30
#endif

// Function pointer types
typedef GLhandleARB(APIENTRY* PFNGLCREATESHADEROBJECTARBPROC)(GLenum shaderType);
typedef void        (APIENTRY* PFNGLSHADERSOURCEARBPROC)(GLhandleARB shaderObj, GLsizei count, const GLcharARB** string, const GLint* length);
typedef void        (APIENTRY* PFNGLCOMPILESHADERARBPROC)(GLhandleARB shaderObj);
typedef GLhandleARB(APIENTRY* PFNGLCREATEPROGRAMOBJECTARBPROC)(void);
typedef void        (APIENTRY* PFNGLATTACHOBJECTARBPROC)(GLhandleARB containerObj, GLhandleARB obj);
typedef void        (APIENTRY* PFNGLLINKPROGRAMARBPROC)(GLhandleARB programObj);
typedef void        (APIENTRY* PFNGLUSEPROGRAMOBJECTARBPROC)(GLhandleARB programObj);
typedef void        (APIENTRY* PFNGLDELETEOBJECTARBPROC)(GLhandleARB obj);
typedef void        (APIENTRY* PFNGLGETOBJECTPARAMETERIVARBPROC)(GLhandleARB obj, GLenum pname, GLint* params);
typedef void        (APIENTRY* PFNGLGETINFOLOGARBPROC)(GLhandleARB obj, GLsizei maxLength, GLsizei* length, GLcharARB* infoLog);
typedef GLint(APIENTRY* PFNGLGETUNIFORMLOCATIONARBPROC)(GLhandleARB programObj, const GLcharARB* name);
typedef void        (APIENTRY* PFNGLUNIFORM1IARBPROC)(GLint location, GLint v0);
typedef void        (APIENTRY* PFNGLUNIFORM1FARBPROC)(GLint location, GLfloat v0);
typedef void        (APIENTRY* PFNGLUNIFORM2FARBPROC)(GLint location, GLfloat v0, GLfloat v1);
typedef void        (APIENTRY* PFNGLUNIFORM3FARBPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void        (APIENTRY* PFNGLUNIFORM4FARBPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void        (APIENTRY* PFNGLUNIFORMMATRIX4FVARBPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

// Loaded pointers
static PFNGLCREATESHADEROBJECTARBPROC      glCreateShaderObjectARB_ = nullptr;
static PFNGLSHADERSOURCEARBPROC            glShaderSourceARB_ = nullptr;
static PFNGLCOMPILESHADERARBPROC           glCompileShaderARB_ = nullptr;
static PFNGLCREATEPROGRAMOBJECTARBPROC     glCreateProgramObjectARB_ = nullptr;
static PFNGLATTACHOBJECTARBPROC            glAttachObjectARB_ = nullptr;
static PFNGLLINKPROGRAMARBPROC             glLinkProgramARB_ = nullptr;
static PFNGLUSEPROGRAMOBJECTARBPROC        glUseProgramObjectARB_ = nullptr;
static PFNGLDELETEOBJECTARBPROC            glDeleteObjectARB_ = nullptr;
static PFNGLGETOBJECTPARAMETERIVARBPROC    glGetObjectParameterivARB_ = nullptr;
static PFNGLGETINFOLOGARBPROC              glGetInfoLogARB_ = nullptr;
static PFNGLGETUNIFORMLOCATIONARBPROC      glGetUniformLocationARB_ = nullptr;
static PFNGLUNIFORM1IARBPROC               glUniform1iARB_ = nullptr;
static PFNGLUNIFORM1FARBPROC               glUniform1fARB_ = nullptr;
static PFNGLUNIFORM2FARBPROC               glUniform2fARB_ = nullptr;
static PFNGLUNIFORM3FARBPROC               glUniform3fARB_ = nullptr;
static PFNGLUNIFORM4FARBPROC               glUniform4fARB_ = nullptr;
static PFNGLUNIFORMMATRIX4FVARBPROC        glUniformMatrix4fvARB_ = nullptr;

namespace bonsai {

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
            const char after = *(pos + extLen);

            if ((before == ' ' || before == '\0') && (after == ' ' || after == '\0'))
                return true;

            start = pos + extLen;
        }
    }

    Shader::~Shader()
    {
        destroy();
    }

    bool Shader::extensionsAvailable() const
    {
        const char* ext = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        // Need the ARB shader object pipeline + vertex/fragment shader
        return hasExtension(ext, "GL_ARB_shader_objects") &&
            hasExtension(ext, "GL_ARB_vertex_shader") &&
            hasExtension(ext, "GL_ARB_fragment_shader");
    }

    bool Shader::ensureLoadedFunctions() const
    {
        if (_funcsLoaded) return true;
        _funcsLoaded = true;

        if (!extensionsAvailable())
            return false;

#if defined(_WIN32)
        auto gp = [](const char* n) -> void* { return (void*)wglGetProcAddress(n); };
#else
        auto gp = [](const char* n) -> void* { return (void*)glXGetProcAddress((const GLubyte*)n); };
#endif

        glCreateShaderObjectARB_ = (PFNGLCREATESHADEROBJECTARBPROC)gp("glCreateShaderObjectARB");
        glShaderSourceARB_ = (PFNGLSHADERSOURCEARBPROC)gp("glShaderSourceARB");
        glCompileShaderARB_ = (PFNGLCOMPILESHADERARBPROC)gp("glCompileShaderARB");
        glCreateProgramObjectARB_ = (PFNGLCREATEPROGRAMOBJECTARBPROC)gp("glCreateProgramObjectARB");
        glAttachObjectARB_ = (PFNGLATTACHOBJECTARBPROC)gp("glAttachObjectARB");
        glLinkProgramARB_ = (PFNGLLINKPROGRAMARBPROC)gp("glLinkProgramARB");
        glUseProgramObjectARB_ = (PFNGLUSEPROGRAMOBJECTARBPROC)gp("glUseProgramObjectARB");
        glDeleteObjectARB_ = (PFNGLDELETEOBJECTARBPROC)gp("glDeleteObjectARB");
        glGetObjectParameterivARB_ = (PFNGLGETOBJECTPARAMETERIVARBPROC)gp("glGetObjectParameterivARB");
        glGetInfoLogARB_ = (PFNGLGETINFOLOGARBPROC)gp("glGetInfoLogARB");
        glGetUniformLocationARB_ = (PFNGLGETUNIFORMLOCATIONARBPROC)gp("glGetUniformLocationARB");
        glUniform1iARB_ = (PFNGLUNIFORM1IARBPROC)gp("glUniform1iARB");
        glUniform1fARB_ = (PFNGLUNIFORM1FARBPROC)gp("glUniform1fARB");
        glUniform2fARB_ = (PFNGLUNIFORM2FARBPROC)gp("glUniform2fARB");
        glUniform3fARB_ = (PFNGLUNIFORM3FARBPROC)gp("glUniform3fARB");
        glUniform4fARB_ = (PFNGLUNIFORM4FARBPROC)gp("glUniform4fARB");
        glUniformMatrix4fvARB_ = (PFNGLUNIFORMMATRIX4FVARBPROC)gp("glUniformMatrix4fvARB");

        const bool ok =
            glCreateShaderObjectARB_ && glShaderSourceARB_ && glCompileShaderARB_ &&
            glCreateProgramObjectARB_ && glAttachObjectARB_ && glLinkProgramARB_ &&
            glUseProgramObjectARB_ && glDeleteObjectARB_ &&
            glGetObjectParameterivARB_ && glGetInfoLogARB_ &&
            glGetUniformLocationARB_ && glUniform1iARB_ && glUniform1fARB_ &&
            glUniform2fARB_ && glUniform3fARB_ && glUniform4fARB_ && glUniformMatrix4fvARB_;

        return ok;
    }

    void Shader::destroy()
    {
        if (_program && ensureLoadedFunctions())
        {
            glUseProgramObjectARB_(0);
            if (_vs) glDeleteObjectARB_(_vs);
            if (_fs) glDeleteObjectARB_(_fs);
            glDeleteObjectARB_(_program);
        }

        _program = 0;
        _vs = 0;
        _fs = 0;
        _uniformCache.clear();
    }

    static std::string infoLog(GLhandleARB obj)
    {
        if (!glGetObjectParameterivARB_ || !glGetInfoLogARB_) return {};

        GLint len = 0;
        glGetObjectParameterivARB_(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB, &len);
        if (len <= 1) return {};

        std::string log;
        log.resize((size_t)len);
        GLsizei outLen = 0;
        glGetInfoLogARB_(obj, (GLsizei)log.size(), &outLen, log.data());
        if (outLen > 0 && (size_t)outLen < log.size())
            log.resize((size_t)outLen);
        return log;
    }

    bool Shader::compileAttachLink(const std::string& vs, const std::string& fs)
    {
        if (!ensureLoadedFunctions())
            return false;

        destroy();

        _program = (unsigned int)glCreateProgramObjectARB_();
        _vs = (unsigned int)glCreateShaderObjectARB_(GL_VERTEX_SHADER_ARB);
        _fs = (unsigned int)glCreateShaderObjectARB_(GL_FRAGMENT_SHADER_ARB);

        const GLcharARB* vsrc = (const GLcharARB*)vs.c_str();
        const GLcharARB* fsrc = (const GLcharARB*)fs.c_str();
        GLint vlen = (GLint)vs.size();
        GLint flen = (GLint)fs.size();

        glShaderSourceARB_((GLhandleARB)_vs, 1, &vsrc, &vlen);
        glCompileShaderARB_((GLhandleARB)_vs);

        GLint vOK = 0;
        glGetObjectParameterivARB_((GLhandleARB)_vs, GL_OBJECT_COMPILE_STATUS_ARB, &vOK);
        if (!vOK)
        {
            std::printf("[bonsai::Shader] Vertex compile failed:\n%s\n", infoLog((GLhandleARB)_vs).c_str());
            destroy();
            return false;
        }

        glShaderSourceARB_((GLhandleARB)_fs, 1, &fsrc, &flen);
        glCompileShaderARB_((GLhandleARB)_fs);

        GLint fOK = 0;
        glGetObjectParameterivARB_((GLhandleARB)_fs, GL_OBJECT_COMPILE_STATUS_ARB, &fOK);
        if (!fOK)
        {
            std::printf("[bonsai::Shader] Fragment compile failed:\n%s\n", infoLog((GLhandleARB)_fs).c_str());
            destroy();
            return false;
        }

        glAttachObjectARB_((GLhandleARB)_program, (GLhandleARB)_vs);
        glAttachObjectARB_((GLhandleARB)_program, (GLhandleARB)_fs);
        glLinkProgramARB_((GLhandleARB)_program);

        GLint linkOK = 0;
        glGetObjectParameterivARB_((GLhandleARB)_program, GL_OBJECT_LINK_STATUS_ARB, &linkOK);
        if (!linkOK)
        {
            std::printf("[bonsai::Shader] Link failed:\n%s\n", infoLog((GLhandleARB)_program).c_str());
            destroy();
            return false;
        }

        return true;
    }

    bool Shader::buildFromSource(const std::string& vertexSrc, const std::string& fragmentSrc)
    {
        return compileAttachLink(vertexSrc, fragmentSrc);
    }

    std::string Shader::loadTextFile(const std::string& path)
    {
        std::ifstream in(path, std::ios::in | std::ios::binary);
        if (!in) return {};
        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

    bool Shader::buildFromFiles(const std::string& vertexPath, const std::string& fragmentPath)
    {
        const std::string vs = loadTextFile(vertexPath);
        const std::string fs = loadTextFile(fragmentPath);
        if (vs.empty() || fs.empty())
            return false;
        return compileAttachLink(vs, fs);
    }

    void Shader::use() const
    {
        if (!_program) return;
        if (!ensureLoadedFunctions()) return;
        glUseProgramObjectARB_((GLhandleARB)_program);
    }

    void Shader::unuse()
    {
        if (glUseProgramObjectARB_)
            glUseProgramObjectARB_(0);
    }

    int Shader::getUniformLocation(const char* name) const
    {
        if (!_program || !name || !*name) return -1;
        if (!ensureLoadedFunctions()) return -1;

        auto it = _uniformCache.find(name);
        if (it != _uniformCache.end())
            return it->second;

        const GLint loc = glGetUniformLocationARB_((GLhandleARB)_program, (const GLcharARB*)name);
        _uniformCache.emplace(name, (int)loc);
        return (int)loc;
    }

    bool Shader::setUniform(const char* name, int v) const
    {
        const int loc = getUniformLocation(name);
        if (loc < 0) return false;
        glUniform1iARB_(loc, v);
        return true;
    }

    bool Shader::setUniform(const char* name, float v) const
    {
        const int loc = getUniformLocation(name);
        if (loc < 0) return false;
        glUniform1fARB_(loc, v);
        return true;
    }

    bool Shader::setUniform(const char* name, const glm::vec2& v) const
    {
        const int loc = getUniformLocation(name);
        if (loc < 0) return false;
        glUniform2fARB_(loc, v.x, v.y);
        return true;
    }

    bool Shader::setUniform(const char* name, const glm::vec3& v) const
    {
        const int loc = getUniformLocation(name);
        if (loc < 0) return false;
        glUniform3fARB_(loc, v.x, v.y, v.z);
        return true;
    }

    bool Shader::setUniform(const char* name, const glm::vec4& v) const
    {
        const int loc = getUniformLocation(name);
        if (loc < 0) return false;
        glUniform4fARB_(loc, v.x, v.y, v.z, v.w);
        return true;
    }

    bool Shader::setUniform(const char* name, const glm::mat4& m) const
    {
        const int loc = getUniformLocation(name);
        if (loc < 0) return false;
        // GL expects column-major; glm is column-major by default; transpose = GL_FALSE
        glUniformMatrix4fvARB_(loc, 1, GL_FALSE, &m[0][0]);
        return true;
    }

    void Shader::apply(const RenderState& state) const
    {
        if (!_program) return;

        const glm::mat4& M = state.model;
        const glm::mat4& V = state.view;
        const glm::mat4& P = state.proj;
        const glm::mat4 MVP = P * V * M;

        // Standard matrices (no-ops if uniforms not present)
        setUniform("uModel", M);
        setUniform("uView", V);
        setUniform("uProj", P);
        setUniform("uMVP", MVP);

        // Standard sampler convention for shader-only multitexture
        setUniform("uTexture0", 0);
        setUniform("uTexture1", 1);
        setUniform("uTexture2", 2);
        setUniform("uTexture3", 3);
    }

} // namespace bonsai