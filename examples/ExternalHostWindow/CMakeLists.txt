add_executable(ExternalHostWindow
  main.cpp
)

target_link_libraries(ExternalHostWindow PRIVATE bonsai)

set_target_properties(ExternalHostWindow PROPERTIES FOLDER "Examples")