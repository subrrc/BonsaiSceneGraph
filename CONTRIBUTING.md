# CONTRIBUTING.md

## Contributing to BonsaiSceneGraph

Thanks for your interest in contributing! BonsaiSceneGraph aims to stay small, readable, and OpenGL 1.4 friendly.

### Guiding Principles
- Keep it **lightweight**: avoid adding big systems or heavy dependencies.
- Prefer **clarity over cleverness**: simple code wins.
- Stay compatible with **OpenGL 1.4 / fixed-function** rendering paths (unless a feature is clearly optional).
- Avoid performance/optimization layers that complicate traversal (culling, sorting, paging, etc.).

## What to Contribute
Good contributions:
- Bug fixes and stability improvements
- Minimal new nodes (e.g., a simple `Transform` node using matrix stack)
- Small improvements to `Geometry` utilities (primitive builders, validation)
- Better examples/tests
- Documentation improvements

Less likely to be accepted:
- Large rendering abstraction layers
- Complex state systems that resemble full engines
- Multi-threaded render pipelines

## Development Setup
### Build (CMake)
```bash
cmake -S . -B build
cmake --build build --config Debug
```

If examples are supported:
```bash
cmake -S . -B build -DBONSAI_BUILD_EXAMPLES=ON
cmake --build build --config Debug
```

## Coding Style
- Keep style consistent with the existing codebase.
- Prefer modern C++ where it simplifies ownership and safety, but keep the library approachable.
- Avoid introducing new third-party dependencies unless strongly justified.

## Submitting Changes
1. Fork the repo and create a branch:
   - `feature/<short-name>` or `fix/<short-name>`
2. Keep commits focused and descriptive.
3. Ensure the project builds on:
   - at minimum: Windows (MSVC) and one of Linux/macOS if possible
4. Open a PR with:
   - what/why summary
   - any API changes (namespace, includes, class changes)
   - screenshots or demo notes for visual changes (if applicable)

## Reporting Issues
When reporting bugs, please include:
- OS and compiler (MSVC/Clang/GCC version)
- CMake generator/version
- Minimal reproduction steps
- Expected vs actual behavior
