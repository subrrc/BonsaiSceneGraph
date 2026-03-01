# BonsaiSceneGraph

A small, lightweight C++ scene graph inspired by the structure of OpenSceneGraph, designed for **simple, single-threaded rendering using OpenGL 1.4** (fixed-function pipeline era).

BonsaiSceneGraph intentionally stays “bonsai-sized”: **no culling, no sorting, no multithreading, and no optimization layers**—just build a node hierarchy and draw it.

## Features
- Minimal OSG-like core: `Node`, `Group`, `Geode`, `Drawable`, `Geometry`
- Simple `Image` container for basic texture usage (**one texture at a time**)
- Straightforward traversal: **visit nodes → draw drawables**
- Single-threaded rendering model
- CMake-based build, friendly with Visual Studio’s CMake integration

## Non-Goals
BonsaiSceneGraph does **not** aim to provide:
- Culling/sorting/batching/LOD/paging
- Multi-threaded update/cull/draw pipelines
- Modern GL/Vulkan abstraction layers
- Heavy state/asset systems

## Quick Start

### Build (CMake)
```bash
cmake -S . -B build
cmake --build build --config Release
```

If your repo includes examples and a build option:
```bash
cmake -S . -B build -DBONSAI_BUILD_EXAMPLES=ON
cmake --build build --config Release
```

### Minimal usage sketch
```cpp
#include <bonsai/Group.h>
#include <bonsai/Geode.h>
#include <bonsai/Geometry.h>
#include <bonsai/Viewer.h>

int main()
{
    using namespace bonsai;

    auto root = std::make_shared<Group>();
    auto geode = std::make_shared<Geode>();

    auto geom = std::make_shared<Geometry>();
    // Fill geometry (verts/normals/colors/texcoords/indices) as your API defines...

    geode->addDrawable(geom);
    root->addChild(geode);

    Viewer viewer;
    viewer.setSceneData(root);
    return viewer.run(); // or viewer.frame() loop, depending on implementation
}
```

## Visual Studio Notes (CMake)
When opened as a CMake project in Visual Studio, header files show in Solution Explorer when they’re added to the target sources (via `target_sources(...)` / `add_library(...)` lists).

## Repository Layout (typical)
- `bonsai/` — library sources/headers
- `examples/` — optional sample programs
- `CMakeLists.txt` — top-level build configuration

## Project Summary
See [`PROJECT_SUMMARY.md`](PROJECT_SUMMARY.md) for a concise overview, goals, and architecture notes.

## Contributing
Contributions are welcome. Please see [`CONTRIBUTING.md`](CONTRIBUTING.md).

## License
MIT License (see [`LICENSE`](LICENSE)).
