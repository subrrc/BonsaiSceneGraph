add_executable(ShaderTextureTriangle
  main.cpp
)

target_link_libraries(ShaderTextureTriangle PRIVATE bonsai)

set_target_properties(ShaderTextureTriangle PROPERTIES FOLDER "Examples")