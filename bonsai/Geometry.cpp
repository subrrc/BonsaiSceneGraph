// bonsai/Geometry.cpp
#include "Geometry.h"
#include "RenderState.h"
#include "Shader.h"

#if defined(_WIN32)
  #include <windows.h>
  #include <GL/gl.h>
#else
  #include <GL/gl.h>
#endif

namespace bonsai {

static void loadMatrixGL(const glm::mat4& m)
{
    glLoadMatrixf(&m[0][0]);
}

unsigned int Geometry::toGL(Primitive p)
{
    switch (p)
    {
        case Primitive::Triangles: return GL_TRIANGLES;
        case Primitive::Lines:     return GL_LINES;
        case Primitive::Points:    return GL_POINTS;
        default:                   return GL_TRIANGLES;
    }
}

void Geometry::setTexture(int unit, std::shared_ptr<Image> img)
{
    if (unit < 0 || unit >= MaxTextureUnits) return;
    _textures[(size_t)unit] = std::move(img);
    markDirty();
}

std::shared_ptr<Image> Geometry::getTexture(int unit) const
{
    if (unit < 0 || unit >= MaxTextureUnits) return {};
    return _textures[(size_t)unit];
}

void Geometry::draw(const RenderState& state)
{
    const bool useShader = (state.shader && state.shader->isValid());
    if (useShader)
    {
        state.shader->use();
        state.shader->apply(state);
    }

    glMatrixMode(GL_MODELVIEW);
    glm::mat4 mv = state.view * state.model;

    glPushMatrix();
    loadMatrixGL(mv);

    // ---- textures (lazy upload on bind) ----
    bool anyTex = false;
    for (int unit = 0; unit < MaxTextureUnits; ++unit)
    {
        if (_textures[(size_t)unit]) { anyTex = true; break; }
    }

    if (anyTex) glEnable(GL_TEXTURE_2D);
    else        glDisable(GL_TEXTURE_2D);

    for (int unit = 0; unit < MaxTextureUnits; ++unit)
    {
        auto& tex = _textures[(size_t)unit];
        if (tex) tex->bind(unit);
    }

    // ---- arrays ----
    if (_verts.empty())
    {
        glPopMatrix();
        if (useShader) Shader::unuse();
        return;
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, _verts.data());

    if (!_normals.empty())
    {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 0, _normals.data());
    }
    else glDisableClientState(GL_NORMAL_ARRAY);

    if (!_uvs.empty())
    {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, _uvs.data());
    }
    else glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    if (!_colors.empty())
    {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_FLOAT, 0, _colors.data());
    }
    else
    {
        glDisableClientState(GL_COLOR_ARRAY);
        glColor4f(1.f, 1.f, 1.f, 1.f);
    }

    GLenum mode = static_cast<GLenum>(toGL(_prim));
    if (!_indices.empty())
        glDrawElements(mode, (GLsizei)_indices.size(), GL_UNSIGNED_INT, _indices.data());
    else
        glDrawArrays(mode, 0, (GLsizei)_verts.size());

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glPopMatrix();

    if (useShader)
        Shader::unuse();
}

} // namespace bonsai
