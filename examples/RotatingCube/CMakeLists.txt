add_executable(RotatingCube
  main.cpp
)

target_link_libraries(RotatingCube PRIVATE bonsai)

set_target_properties(RotatingCube PROPERTIES FOLDER "Examples")