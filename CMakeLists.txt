# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("ConsoleThing")

# Add source to this project's executable.
add_executable (ConsoleThing WIN32 "ConsoleThing.c" "ConsoleThing.h")
target_link_libraries(ConsoleThing winmm Xinput)
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ConsoleThing PROPERTY CXX_STANDARD 20)
endif()