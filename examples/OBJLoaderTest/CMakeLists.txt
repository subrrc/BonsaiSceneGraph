add_executable(OBJLoaderTest
  main.cpp
)

target_link_libraries(OBJLoaderTest PRIVATE bonsai)

set_target_properties(OBJLoaderTest PROPERTIES FOLDER "Examples")