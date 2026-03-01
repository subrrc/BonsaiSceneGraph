add_executable(PhongPerMaterial
  main.cpp
)

target_link_libraries(PhongPerMaterial PRIVATE bonsai)

set_target_properties(PhongPerMaterial PROPERTIES FOLDER "Examples")