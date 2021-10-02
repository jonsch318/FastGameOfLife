# CMakeList.txt : CMake project for GameOfLife, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (GameOfLife "GameOfLife.cpp" )

# TODO: Add tests and install targets if needed.

target_link_libraries(GameOfLife
	glfw
	libglew_static
)