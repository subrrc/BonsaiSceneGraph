#pragma once
#include <memory>
#include <string>

#include "Node.h"
#include "BoundsUtils.h" // for bonsai::util::Bounds

namespace bonsai {

    // Small OBJ loader: v/vt/vn/f plus o/g grouping and usemtl splitting.
    // Builds a Group root containing one Group per material.
    // Faces are triangulated and expanded (non-indexed triangles).
    class ObjLoader
    {
    public:
        struct Options
        {
            bool loadTexcoords{ true };
            bool loadNormals{ true };

            // If OBJ has no normals and this is true, compute flat normals per triangle.
            bool computeNormalsIfMissing{ true };

            // Flip V texture coordinate
            bool flipV{ false };

            // Apply a uniform scale to positions
            float scale{ 1.0f };

            // NEW: if true, create Group-per-material using usemtl (default true).
            bool groupByMaterial{ true };
        };

        // Returns nullptr on failure.
        // Always returns a Group root if successful.
        // If outBounds != nullptr, fills bounds for the entire loaded model (model space).
        static NodePtr load(const std::string& path,
            const Options& opt = Options{},
            bonsai::util::Bounds* outBounds = nullptr);
    };

} // namespace bonsai