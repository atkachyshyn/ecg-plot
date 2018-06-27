# Minimum CMake version required to generate
# our build system
cmake_minimum_required(VERSION 3.0)

# Name of our Project
project(opengl_test)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)

pkg_search_module(GLFW REQUIRED glfw3)
	
include_directories(${GLFW_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS})

add_library(plotter STATIC plotter.c)
# add_library(adc STATIC adc.c)

# add_executable creates an executable with given name (ECGPlot).
# Source files are given as parameters.
add_executable(ecg_plot main.c)

# target_link_libraries(ecg_plot plotter adc ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES})
target_link_libraries(ecg_plot plotter ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES})
