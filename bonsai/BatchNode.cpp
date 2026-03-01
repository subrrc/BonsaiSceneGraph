#include "BatchNode.h"

#if defined(_WIN32)
  #include <windows.h>
  #include <GL/gl.h>
#else
  #include <GL/gl.h>
#endif

#include <functional>
#include <cmath>

#include "RenderState.h"
#include "Geode.h"
#include "Geometry.h"
#include "Shader.h"
#include "Image.h"

namespace bonsai {

static void loadMatrixGL(const glm::mat4& m) { glLoadMatrixf(&m[0][0]); }

static GLenum primToGL(Geometry::Primitive p)
{
    switch (p)
    {
        case Geometry::Primitive::Triangles: return GL_TRIANGLES;
        case Geometry::Primitive::Lines:     return GL_LINES;
        case Geometry::Primitive::Points:    return GL_POINTS;
        default:                             return GL_TRIANGLES;
    }
}

BatchNode::~BatchNode()
{
    if (_listId != 0)
    {
        glDeleteLists(_listId, 1);
        _listId = 0;
    }
}

void BatchNode::markDirty()
{
    _dirty = true;
    bumpRevision();
}

void BatchNode::addChild(const NodePtr& child)
{
    Group::addChild(child);
    markDirty();
}

void BatchNode::removeChild(const NodePtr& child)
{
    Group::removeChild(child);
    markDirty();
}

void BatchNode::insertChild(std::size_t index, const NodePtr& child)
{
    Group::insertChild(index, child);
    markDirty();
}

void BatchNode::traverse(RenderState& state)
{
    const glm::mat4 prevModel = state.model;

    if (hasTransform())
        state.model = state.model * getTransform();

    rebuildIfNeeded();

    glMatrixMode(GL_MODELVIEW);
    glm::mat4 mv = state.view * state.model;

    glPushMatrix();
    loadMatrixGL(mv);

    if (state.shader && state.shader->isValid())
    {
        state.shader->use();
        state.shader->apply(state);
    }

    if (_listId != 0)
        glCallList(_listId);

    if (state.shader && state.shader->isValid())
        Shader::unuse();

    glPopMatrix();

    state.model = prevModel;
}

void BatchNode::rebuildIfNeeded()
{
    const std::uint64_t h = computeSubtreeHash();
    if (_dirty || _listId == 0 || h != _compiledHash)
    {
        rebuildDisplayList();
        _compiledHash = h;
        _dirty = false;
    }
}

std::uint64_t BatchNode::computeSubtreeHash() const
{
    const std::uint64_t FNV_OFF = 1469598103934665603ull;
    const std::uint64_t FNV_PR  = 1099511628211ull;

    std::uint64_t hash = FNV_OFF;

    std::function<void(const NodePtr&)> walk = [&](const NodePtr& n)
    {
        if (!n) return;

        hash ^= n->revision(); hash *= FNV_PR;

        if (auto geode = std::dynamic_pointer_cast<Geode>(n))
        {
            for (const auto& d : geode->getDrawables())
            {
                if (auto g = std::dynamic_pointer_cast<Geometry>(d))
                {
                    hash ^= g->revision(); hash *= FNV_PR;

                    // Stable identity only (DO NOT hash GL textureId())
                    auto t0 = g->getTexture(0);
                    std::uintptr_t p = reinterpret_cast<std::uintptr_t>(t0.get());
                    hash ^= (std::uint64_t)p; hash *= FNV_PR;
                }
                else
                {
                    std::uintptr_t p = reinterpret_cast<std::uintptr_t>(d.get());
                    hash ^= (std::uint64_t)p; hash *= FNV_PR;
                }
            }
        }

        if (auto grp = std::dynamic_pointer_cast<Group>(n))
        {
            for (const auto& c : grp->getChildren())
                walk(c);
        }
    };

    for (const auto& c : _children)
        walk(c);

    hash ^= (std::uint64_t)_includeTextured;   hash *= FNV_PR;
    hash ^= (std::uint64_t)_includeUntextured; hash *= FNV_PR;

    return hash;
}

void BatchNode::collectGeometry(const NodePtr& node, const glm::mat4& parentModel, std::vector<Item>& out) const
{
    if (!node) return;

    if (std::dynamic_pointer_cast<const BatchNode>(node))
        return;

    glm::mat4 M = parentModel;
    if (node->hasTransform())
        M = parentModel * node->getTransform();

    if (auto geode = std::dynamic_pointer_cast<Geode>(node))
    {
        for (const auto& d : geode->getDrawables())
        {
            auto g = std::dynamic_pointer_cast<Geometry>(d);
            if (!g) continue;

            const bool hasTex = (bool)g->getTexture(0);
            if (hasTex && !_includeTextured) continue;
            if (!hasTex && !_includeUntextured) continue;

            out.push_back(Item{g, M});
        }
    }

    if (auto grp = std::dynamic_pointer_cast<Group>(node))
    {
        for (const auto& c : grp->getChildren())
            collectGeometry(c, M, out);
    }
}

void BatchNode::rebuildDisplayList()
{
    std::vector<Item> items;
    items.reserve(256);

    for (const auto& c : _children)
        collectGeometry(c, glm::mat4(1.0f), items);

    if (_listId == 0)
        _listId = glGenLists(1);

    glNewList(_listId, GL_COMPILE);

    for (const auto& it : items)
    {
        const auto& geom = it.geom;
        if (!geom) continue;

        const auto& V  = geom->vertices();
        if (V.empty()) continue;

        const auto& N  = geom->normals();
        const auto& UV = geom->texcoords();
        const auto& C  = geom->colors();
        const auto& I  = geom->indices();

        auto tex0 = geom->getTexture(0);
        if (tex0)
        {
            glEnable(GL_TEXTURE_2D);
            tex0->bind(0);
        }
        else
        {
            glDisable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        const GLenum mode = primToGL(geom->primitive());
        glBegin(mode);

        auto emit = [&](std::size_t idx)
        {
            const glm::vec4 p4 = it.localModel * glm::vec4(V[idx], 1.0f);
            const glm::vec3 p  = glm::vec3(p4);

            if (idx < N.size())
            {
                glm::vec3 n = glm::mat3(it.localModel) * N[idx];
                const float len = std::sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
                if (len > 0.0f) n /= len;
                glNormal3f(n.x, n.y, n.z);
            }

            if (idx < C.size())
            {
                const auto& c = C[idx];
                glColor4f(c.r, c.g, c.b, c.a);
            }
            else
            {
                glColor4f(1.f, 1.f, 1.f, 1.f);
            }

            if (idx < UV.size())
            {
                const auto& uv = UV[idx];
                glTexCoord2f(uv.x, uv.y);
            }

            glVertex3f(p.x, p.y, p.z);
        };

        if (!I.empty())
        {
            for (std::size_t k = 0; k < I.size(); ++k)
            {
                const std::size_t idx = (std::size_t)I[k];
                if (idx < V.size()) emit(idx);
            }
        }
        else
        {
            for (std::size_t idx = 0; idx < V.size(); ++idx)
                emit(idx);
        }

        glEnd();
    }

    glEndList();
}

} // namespace bonsai
