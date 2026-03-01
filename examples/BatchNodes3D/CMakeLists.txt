add_executable(BatchNodes3D
  main.cpp
)

target_link_libraries(BatchNodes3D PRIVATE bonsai)

set_target_properties(BatchNodes3D PROPERTIES FOLDER "Examples")