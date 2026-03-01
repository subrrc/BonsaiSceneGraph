add_executable(PhongLoader
  main.cpp
)

target_link_libraries(PhongLoader PRIVATE bonsai)

set_target_properties(PhongLoader PROPERTIES FOLDER "Examples")