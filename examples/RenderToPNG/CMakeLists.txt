add_executable(RenderToPNG
  main.cpp
)

target_link_libraries(RenderToPNG PRIVATE bonsai)

set_target_properties(RenderToPNG PROPERTIES FOLDER "Examples")