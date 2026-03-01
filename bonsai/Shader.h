#pragma once
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace bonsai {

    struct RenderState;

    class Shader
    {
    public:
        Shader() = default;
        ~Shader();

        // Build from GLSL source strings. Returns true on success.
        bool buildFromSource(const std::string& vertexSrc, const std::string& fragmentSrc);

        // Convenience: build from files on disk.
        bool buildFromFiles(const std::string& vertexPath, const std::string& fragmentPath);

        // Bind / unbind program
        void use() const;
        static void unuse();

        bool isValid() const { return _program != 0; }
        unsigned int programId() const { return _program; }

        // Apply standard RenderState uniforms if present:
        // uModel, uView, uProj, uMVP, uNormalMatrix (optional), uTexture0 (optional)
        void apply(const RenderState& state) const;

        // Uniform helpers (return false if uniform not found or shader invalid)
        bool setUniform(const char* name, int v) const;
        bool setUniform(const char* name, float v) const;
        bool setUniform(const char* name, const glm::vec2& v) const;
        bool setUniform(const char* name, const glm::vec3& v) const;
        bool setUniform(const char* name, const glm::vec4& v) const;
        bool setUniform(const char* name, const glm::mat4& m) const;

    private:
        // non-copyable (simple)
        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        bool ensureLoadedFunctions() const;
        bool extensionsAvailable() const;

        bool compileAttachLink(const std::string& vs, const std::string& fs);
        static std::string loadTextFile(const std::string& path);

        int getUniformLocation(const char* name) const;

        void destroy();

    private:
        mutable bool _funcsLoaded{ false };
        unsigned int _program{ 0 };
        unsigned int _vs{ 0 };
        unsigned int _fs{ 0 };

        mutable std::unordered_map<std::string, int> _uniformCache;
    };

} // namespace bonsai