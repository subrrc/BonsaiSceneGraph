# PROJECT_SUMMARY.md

## Overview
**BonsaiSceneGraph** is a small, lightweight C++ scene graph inspired by the structure of OpenSceneGraph, designed specifically for **simple, single-threaded rendering using OpenGL 1.4** (fixed-function pipeline era). It focuses on clarity and minimal moving parts: **no culling, no sorting, no multithreading, and no optimization layers**—just build a node hierarchy and draw it.

The library is intended for learning, prototyping, legacy OpenGL contexts, and small tools where a full-featured engine or heavyweight scene graph is unnecessary.

## Goals
- Provide a **minimal scene graph** with familiar OSG-like concepts
- Keep the implementation **small and easy to understand**
- Support **single-threaded rendering**
- Use **OpenGL 1.4** compatible drawing paths
- Prioritize **straightforward traversal and drawing** over performance systems

## Non-Goals
BonsaiSceneGraph intentionally does **not** implement:
- Scene graph optimizations (culling, sorting, batching, LOD, paging)
- Multithreaded update/cull/draw pipelines
- Advanced state management systems
- Modern OpenGL/Vulkan abstraction layers

## Core Concepts and Classes
All public API is under:

```cpp
namespace bonsai { ... }
```

Primary classes follow a classic scene graph model:

- **Node**  
  Base scene graph element. Supports traversal and common behavior.
- **Group**  
  A `Node` that owns children. Used for hierarchy and traversal.
- **Geode**  
  A leaf-like `Node` that contains one or more drawables.
- **Drawable**  
  Renderable interface/base that can be attached to a `Geode`.
- **Geometry**  
  A `Drawable` for vertex-based geometry (positions, normals, colors, texcoords, indices).
- **Image** (Texture support)  
  A small image container used for **one texture at a time** use cases.
- **Viewer**  
  Lightweight draw driver that traverses the scene and issues draw calls.

## Typical Usage
- Construct geometry/drawables
- Attach drawables to a `Geode`
- Build a hierarchy with `Group`
- Pass the root node to the `Viewer`
- Call the draw loop

Traversal model is intentionally direct: **visit nodes → draw drawables**.

## Build
BonsaiSceneGraph is built with **CMake**:

```bash
cmake -S . -B build
cmake --build build --config Release
```

If examples exist, they may be controlled with:
- `BONSAI_BUILD_EXAMPLES=ON/OFF`

## Visual Studio (CMake Project)
When opened as a CMake project in Visual Studio:
- Header files appear in Solution Explorer when they are listed in the target sources.

## Design Notes
- The code favors **simple ownership and traversal**
- Rendering is **single-threaded**
- The API is intentionally small so you can extend it:
  - minimal `Transform` node (matrix stack push/pop)
  - simple `StateSet`-style container for GL enables/material
  - additional drawable types and primitive helpers
  - basic texture loading helpers for `Image`
