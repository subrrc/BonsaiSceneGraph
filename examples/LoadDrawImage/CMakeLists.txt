add_executable(LoadDrawImage
  main.cpp
)

target_link_libraries(LoadDrawImage PRIVATE bonsai)

set_target_properties(LoadDrawImage PROPERTIES FOLDER "Examples")