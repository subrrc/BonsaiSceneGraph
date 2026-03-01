# ARCHITECTURE.md

This document describes the minimal traversal and rendering model used by BonsaiSceneGraph.

## High-Level Flow

BonsaiSceneGraph follows a direct scene graph approach:

1. You create a node hierarchy rooted at a `bonsai::Node` (commonly a `bonsai::Group`).
2. Leaf-ish nodes (`bonsai::Geode`) hold one or more `bonsai::Drawable` instances.
3. The `bonsai::Viewer` traverses the hierarchy and asks drawables to render.

There is intentionally no optimization stage (no cull traversal, no sort, no batching).

## Core Objects

### Node
The base type for objects in the scene graph. A `Node` participates in traversal.

Typical responsibilities:
- Provide a virtual traversal interface (e.g., `accept(Visitor&)` or `traverse(...)`)
- Hold basic metadata (optional): name, masks, etc.

### Group
A `Node` that owns children nodes.
- `addChild(std::shared_ptr<Node>)`
- Traversal visits children in insertion order.

### Geode
A `Node` that owns one or more `Drawable` objects.
- `addDrawable(std::shared_ptr<Drawable>)`
- During traversal, the geode draws each drawable in order.

### Drawable
Renderable interface/base class.
- Exposes a draw call invoked during traversal (commonly `draw()` or `draw(RenderInfo&)`).

### Geometry
A concrete `Drawable` that stores vertex data and issues OpenGL 1.4 style draw calls.
Typical data (depending on your implementation):
- positions, normals, colors, texcoords
- optional index buffer
- primitive type

### Image (Texture)
A small image container intended to support basic texture usage (one texture at a time).
It typically holds:
- width/height
- pixel format
- pixel bytes

## Viewer

The `Viewer` is responsible for:
- creating/owning the platform window/context (depending on your implementation)
- setting the scene root (`setSceneData(...)`)
- calling traversal per-frame (e.g., `frame()` / `run()`)

The Viewer generally does:
- clear framebuffer
- set basic GL state
- traverse root node (draw)
- swap buffers

## Traversal Model

The simplest mental model is:

- `Viewer` calls `root->draw()` (or `traverse`)
- `Group` calls `child->draw()` for each child
- `Geode` calls `drawable->draw()` for each drawable
- `Geometry` issues GL commands

This direct model is easy to debug and extend.

## Extension Points (still “bonsai-sized”)

Suggestions that keep the library lightweight:
- `Transform` node using matrix stack push/pop
- a tiny `State`/`StateSet` object to bundle GL enables/material params
- helper builders for common primitives (quad, cube, grid)
- a tiny loader for images (optional, behind a build option)
