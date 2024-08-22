cmake_minimum_required(VERSION 3.10)
project(OpenGLProject)

set(CMAKE_CXX_STANDARD 17)

# Include GLFW
add_subdirectory(libs/GLFW)

# Include GLAD
add_library(glad libs/GLAD/src/glad.c)
target_include_directories(glad PUBLIC libs/GLAD/include)

# Include directories
include_directories(libs/GLFW/include libs/GLAD/include)

# Add source files
add_executable(OpenGLProject src/main.cpp)

# Link libraries
target_link_libraries(OpenGLProject glfw glad)
