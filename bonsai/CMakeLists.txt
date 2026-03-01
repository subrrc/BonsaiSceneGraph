add_library(bonsai STATIC
  Drawable.cpp
  Node.cpp
  Group.cpp
  Geode.cpp
  Image.cpp
  Geometry.cpp
  Shader.cpp
  Viewer.cpp
  TestUtils.cpp
  ObjLoader.cpp
  BoundsUtils.cpp
  ImageLoader.cpp
  PngLoader.cpp
  WicImageLoader.cpp
  JpegLoader.cpp
  TiffLoader.cpp
  BmpLoader.cpp
  SgiRgbLoader.cpp
  UniformDrawable.cpp
  BatchNode.cpp
)

# Consumers (examples) just link to bonsai and get headers automatically.
target_include_directories(bonsai PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# glm is header-only; prefer an installed package, else fall back to vendored external/glm
find_package(glm CONFIG QUIET)
if (glm_FOUND)
  target_link_libraries(bonsai PUBLIC glm::glm)
else()
  if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../external/glm/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../external/glm external_glm_build)
    target_link_libraries(bonsai PUBLIC glm::glm)
  else()
    message(FATAL_ERROR
      "glm not found. Install glm (CONFIG package) or vendor it at external/glm.")
  endif()
endif()

if (WIN32)
  target_link_libraries(bonsai PUBLIC opengl32 windowscodecs)
endif()
