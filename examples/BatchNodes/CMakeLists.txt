add_executable(BatchNodes
  main.cpp
)

target_link_libraries(BatchNodes PRIVATE bonsai)

set_target_properties(BatchNodes PROPERTIES FOLDER "Examples")