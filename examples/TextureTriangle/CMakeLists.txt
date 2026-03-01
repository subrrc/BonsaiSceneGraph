add_executable(TextureTriangle
  main.cpp
)

target_link_libraries(TextureTriangle PRIVATE bonsai)

set_target_properties(TextureTriangle PROPERTIES FOLDER "Examples")