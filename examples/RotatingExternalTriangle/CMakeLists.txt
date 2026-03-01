add_executable(RotatingExternalTriangle
  main.cpp
)

target_link_libraries(RotatingExternalTriangle PRIVATE bonsai)


set_target_properties(RotatingExternalTriangle PROPERTIES FOLDER "Examples")