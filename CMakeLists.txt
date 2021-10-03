# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)


project ("GameOfLife")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/CMakeFiles/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/CMakeFiles/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/CMakeFiles/build)


add_subdirectory("include/glfw/")
add_subdirectory("include/glew/")
# add_subdirectory("include/glm/")

# Include sub-projects.
add_subdirectory ("src")

target_include_directories(GameOfLife PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/glfw/include)
target_include_directories(GameOfLife PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/glew/include)