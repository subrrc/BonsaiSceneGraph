add_executable(RotatingTriangle
  main.cpp
)

target_link_libraries(RotatingTriangle PRIVATE bonsai)

set_target_properties(RotatingTriangle PROPERTIES FOLDER "Examples")