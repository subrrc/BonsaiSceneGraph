#include "ObjLoader.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cctype>
#include <cstdlib>
#include <algorithm>
#include <limits>
#include <unordered_map>

#include <glm/glm.hpp>

#include "Group.h"
#include "Geode.h"
#include "Geometry.h"

namespace bonsai {

    static inline std::string trim(const std::string& s)
    {
        size_t a = 0, b = s.size();
        while (a < b && std::isspace((unsigned char)s[a])) ++a;
        while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
        return s.substr(a, b - a);
    }

    struct ObjIndex
    {
        int v{ -1 };
        int vt{ -1 };
        int vn{ -1 };
    };

    static bool parseIndexToken(const std::string& tok, ObjIndex& out)
    {
        // Supported: v | v/vt | v//vn | v/vt/vn
        out = ObjIndex{};

        int parts[3] = { 0, 0, 0 };
        bool has[3] = { false, false, false };

        int part = 0;
        std::string cur;
        for (size_t i = 0; i <= tok.size(); ++i)
        {
            char c = (i < tok.size()) ? tok[i] : '\0';
            if (c == '/' || c == '\0')
            {
                if (!cur.empty())
                {
                    parts[part] = std::atoi(cur.c_str());
                    has[part] = true;
                }
                cur.clear();
                ++part;
                if (c == '\0') break;
                if (part > 2) break;
            }
            else
            {
                cur.push_back(c);
            }
        }

        if (!has[0]) return false;

        out.v = parts[0];
        out.vt = has[1] ? parts[1] : 0;
        out.vn = has[2] ? parts[2] : 0;
        return true;
    }

    static int resolveIndex(int idx, int count)
    {
        // OBJ: 1..count, or negative for relative to end: -1 == last
        if (idx > 0) return idx - 1;
        if (idx < 0) return count + idx;
        return -1; // 0 or missing
    }

    static glm::vec3 computeFlatNormal(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
    {
        glm::vec3 n = glm::cross(b - a, c - a);
        float len = glm::length(n);
        if (len > 0.0f) n /= len;
        return n;
    }

    // Simple bounds accumulator (model space)
    struct BoundsAcc
    {
        glm::vec3 mn{ std::numeric_limits<float>::max() };
        glm::vec3 mx{ std::numeric_limits<float>::lowest() };
        bool any{ false };

        void expand(const glm::vec3& p)
        {
            mn.x = (std::min)(mn.x, p.x);
            mn.y = (std::min)(mn.y, p.y);
            mn.z = (std::min)(mn.z, p.z);

            mx.x = (std::max)(mx.x, p.x);
            mx.y = (std::max)(mx.y, p.y);
            mx.z = (std::max)(mx.z, p.z);

            any = true;
        }
    };

    static std::shared_ptr<Geode> makeGeodeWithGeometry(std::shared_ptr<Geometry>& outGeom)
    {
        auto geode = std::make_shared<Geode>();
        outGeom = std::make_shared<Geometry>();
        outGeom->setPrimitive(Geometry::Primitive::Triangles);
        geode->addDrawable(outGeom);
        return geode;
    }

    static bool geometryHasData(const std::shared_ptr<Geometry>& g)
    {
        return g && !g->vertices().empty();
    }

    NodePtr ObjLoader::load(const std::string& path,
        const Options& opt,
        bonsai::util::Bounds* outBounds)
    {
        std::ifstream in(path);
        if (!in)
            return nullptr;

        // Source pools
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> texcoords;
        std::vector<glm::vec3> normals;

        // Root always a Group
        auto root = std::make_shared<Group>();
        root->setName("obj_root");

        // Material groups (under root)
        std::unordered_map<std::string, std::shared_ptr<Group>> materialGroups;

        auto getMaterialGroup = [&](const std::string& mtlName) -> std::shared_ptr<Group>
            {
                if (!opt.groupByMaterial)
                    return root; // put everything directly under root if disabled

                auto it = materialGroups.find(mtlName);
                if (it != materialGroups.end())
                    return it->second;

                auto g = std::make_shared<Group>();
                g->setName(std::string("mtl_") + mtlName);
                root->addChild(g);
                materialGroups.emplace(mtlName, g);
                return g;
            };

        // Current state: material + current part geometry under that material group
        std::string currentMtl = "default";
        auto currentParentGroup = getMaterialGroup(currentMtl);

        std::shared_ptr<Geometry> curGeom;
        auto curGeode = makeGeodeWithGeometry(curGeom);
        int partIndex = 0;
        curGeode->setName("obj_part_" + std::to_string(partIndex));

        // Bounds across the whole file (model space)
        BoundsAcc acc;

        auto finalizeCurrentPartIfNeeded = [&]()
            {
                if (geometryHasData(curGeom))
                    currentParentGroup->addChild(curGeode);

                ++partIndex;
                curGeode = makeGeodeWithGeometry(curGeom);
                curGeode->setName("obj_part_" + std::to_string(partIndex));
            };

        auto switchMaterial = [&](const std::string& newMtl)
            {
                if (!opt.groupByMaterial)
                {
                    // Still finalize part so we don't mix materials into one geometry
                    finalizeCurrentPartIfNeeded();
                    currentMtl = newMtl.empty() ? "default" : newMtl;
                    currentParentGroup = root;
                    return;
                }

                // Finish current geometry under previous material group
                finalizeCurrentPartIfNeeded();

                currentMtl = newMtl.empty() ? "default" : newMtl;
                currentParentGroup = getMaterialGroup(currentMtl);
            };

        std::string line;
        while (std::getline(in, line))
        {
            line = trim(line);
            if (line.empty() || line[0] == '#')
                continue;

            std::istringstream ss(line);
            std::string tag;
            ss >> tag;

            if (tag == "mtllib")
            {
                // Ignored (lightweight loader). You can parse later if desired.
                continue;
            }

            if (tag == "usemtl")
            {
                std::string m;
                ss >> m;
                switchMaterial(m);
                continue;
            }

            if (tag == "o" || tag == "g")
            {
                // New object/group: finalize current part and start fresh
                finalizeCurrentPartIfNeeded();
                continue;
            }

            if (tag == "v")
            {
                glm::vec3 p{};
                ss >> p.x >> p.y >> p.z;
                p *= opt.scale;
                positions.push_back(p);
            }
            else if (tag == "vt")
            {
                glm::vec2 uv{};
                ss >> uv.x >> uv.y;
                if (opt.flipV) uv.y = 1.0f - uv.y;
                texcoords.push_back(uv);
            }
            else if (tag == "vn")
            {
                glm::vec3 n{};
                ss >> n.x >> n.y >> n.z;
                float len = glm::length(n);
                if (len > 0.0f) n /= len;
                normals.push_back(n);
            }
            else if (tag == "f")
            {
                // Parse face vertices then triangulate with fan: (0, i, i+1)
                std::vector<ObjIndex> face;
                std::string tok;
                while (ss >> tok)
                {
                    ObjIndex idx;
                    if (parseIndexToken(tok, idx))
                        face.push_back(idx);
                }

                if (face.size() < 3)
                    continue;

                auto emitVertex = [&](const ObjIndex& idx,
                    glm::vec3& p,
                    glm::vec2& uv,
                    glm::vec3& n,
                    bool& hasUV,
                    bool& hasN)
                    {
                        int vi = resolveIndex(idx.v, (int)positions.size());
                        int vti = resolveIndex(idx.vt, (int)texcoords.size());
                        int vni = resolveIndex(idx.vn, (int)normals.size());

                        p = (vi >= 0 && vi < (int)positions.size()) ? positions[(size_t)vi] : glm::vec3(0);

                        hasUV = (opt.loadTexcoords && vti >= 0 && vti < (int)texcoords.size());
                        uv = hasUV ? texcoords[(size_t)vti] : glm::vec2(0);

                        hasN = (opt.loadNormals && vni >= 0 && vni < (int)normals.size());
                        n = hasN ? normals[(size_t)vni] : glm::vec3(0, 0, 1);
                    };

                for (size_t i = 1; i + 1 < face.size(); ++i)
                {
                    glm::vec3 p0, p1, p2;
                    glm::vec2 uv0, uv1, uv2;
                    glm::vec3 n0, n1, n2;
                    bool hasUV0, hasUV1, hasUV2;
                    bool hasN0, hasN1, hasN2;

                    emitVertex(face[0], p0, uv0, n0, hasUV0, hasN0);
                    emitVertex(face[i], p1, uv1, n1, hasUV1, hasN1);
                    emitVertex(face[i + 1], p2, uv2, n2, hasUV2, hasN2);

                    // Expand triangles (non-indexed)
                    curGeom->vertices().push_back(p0);
                    curGeom->vertices().push_back(p1);
                    curGeom->vertices().push_back(p2);

                    // Bounds
                    acc.expand(p0);
                    acc.expand(p1);
                    acc.expand(p2);

                    // UVs
                    if (opt.loadTexcoords && (hasUV0 || hasUV1 || hasUV2))
                    {
                        curGeom->texcoords().push_back(uv0);
                        curGeom->texcoords().push_back(uv1);
                        curGeom->texcoords().push_back(uv2);
                    }

                    // Normals
                    if (opt.loadNormals)
                    {
                        if (hasN0 && hasN1 && hasN2)
                        {
                            curGeom->normals().push_back(n0);
                            curGeom->normals().push_back(n1);
                            curGeom->normals().push_back(n2);
                        }
                        else if (opt.computeNormalsIfMissing)
                        {
                            glm::vec3 fn = computeFlatNormal(p0, p1, p2);
                            curGeom->normals().push_back(fn);
                            curGeom->normals().push_back(fn);
                            curGeom->normals().push_back(fn);
                        }
                    }
                }
            }
            // ignore others: s, etc.
        }

        // Finalize last part
        if (geometryHasData(curGeom))
            currentParentGroup->addChild(curGeode);

        // If grouping by material, root children are material groups; if not, root children are geodes.
        if (root->getChildren().empty())
            return nullptr;

        // Output bounds if requested
        if (outBounds && acc.any)
        {
            outBounds->min = acc.mn;
            outBounds->max = acc.mx;
            outBounds->center = (acc.mn + acc.mx) * 0.5f;
            outBounds->radius = glm::length(acc.mx - outBounds->center);
            if (outBounds->radius < 1e-6f) outBounds->radius = 1.0f;
        }

        return root;
    }

} // namespace bonsai