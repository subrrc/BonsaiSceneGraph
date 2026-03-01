#pragma once
#include <vector>
#include <array>
#include <memory>
#include <cstdint>
#include <glm/glm.hpp>

#include "Drawable.h"
#include "Image.h"

namespace bonsai {

class Geometry : public Drawable
{
public:
    enum class Primitive
    {
        Triangles,
        Lines,
        Points
    };

    static constexpr int MaxTextureUnits = 4;

    Geometry() = default;
    ~Geometry() override = default;

    // Primitive
    void setPrimitive(Primitive p) { _prim = p; markDirty(); }
    Primitive primitive() const { return _prim; }

    // Revision tracking (BatchNode uses this)
    std::uint64_t revision() const { return _revision; }
    void markDirty() { ++_revision; }

    // -----------------------
    // Accessors (NO auto-dirty)
    // -----------------------
    // These do NOT call markDirty(). If you mutate via these, call markDirty() yourself.
    std::vector<glm::vec3>& vertices()  { return _verts; }
    std::vector<glm::vec3>& normals()   { return _normals; }
    std::vector<glm::vec2>& texcoords() { return _uvs; }
    std::vector<glm::vec4>& colors()    { return _colors; }
    std::vector<unsigned int>& indices(){ return _indices; }

    const std::vector<glm::vec3>& vertices()  const { return _verts; }
    const std::vector<glm::vec3>& normals()   const { return _normals; }
    const std::vector<glm::vec2>& texcoords() const { return _uvs; }
    const std::vector<glm::vec4>& colors()    const { return _colors; }
    const std::vector<unsigned int>& indices()const { return _indices; }

    // -----------------------
    // Safe setters (auto-dirty)
    // -----------------------
    void setVertices(std::vector<glm::vec3> v)  { _verts = std::move(v); markDirty(); }
    void setNormals(std::vector<glm::vec3> n)   { _normals = std::move(n); markDirty(); }
    void setTexcoords(std::vector<glm::vec2> u) { _uvs = std::move(u); markDirty(); }
    void setColors(std::vector<glm::vec4> c)    { _colors = std::move(c); markDirty(); }
    void setIndices(std::vector<unsigned int> i){ _indices = std::move(i); markDirty(); }

    // Textures
    void setTexture(int unit, std::shared_ptr<Image> img);
    std::shared_ptr<Image> getTexture(int unit) const;
    void setTexture(std::shared_ptr<Image> img) { setTexture(0, std::move(img)); }

    void draw(const RenderState& state) override;

private:
    Primitive _prim{Primitive::Triangles};

    std::vector<glm::vec3> _verts;
    std::vector<glm::vec3> _normals;
    std::vector<glm::vec2> _uvs;
    std::vector<glm::vec4> _colors;
    std::vector<unsigned int> _indices;

    std::array<std::shared_ptr<Image>, MaxTextureUnits> _textures{};

    std::uint64_t _revision{1};

    static unsigned int toGL(Primitive p);
};

using GeometryPtr = std::shared_ptr<Geometry>;

} // namespace bonsai
