cmake_minimum_required(VERSION 3.16)

project(BonsaiSceneGraph LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(BONSAI_BUILD_EXAMPLES "Build the examples" ON)

# Build the BonsaiSceneGraph library
add_subdirectory(bonsai)

# Build examples (optional)
if (BONSAI_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()