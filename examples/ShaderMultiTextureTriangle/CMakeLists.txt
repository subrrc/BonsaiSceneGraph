add_executable(ShaderMultiTextureTriangle
  main.cpp
)

target_link_libraries(ShaderMultiTextureTriangle PRIVATE bonsai)

set_target_properties(ShaderMultiTextureTriangle PROPERTIES FOLDER "Examples")