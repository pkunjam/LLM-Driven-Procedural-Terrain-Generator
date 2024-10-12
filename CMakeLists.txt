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
include_directories(glm)
include_directories(src/)

# Add source files
add_executable(OpenGLProject 
    src/main.cpp 
    src/UserPrompt.cpp
)

# Link libraries
target_link_libraries(OpenGLProject glfw glad)

# set(CMAKE_TOOLCHAIN_FILE "vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# find_package(CURL REQUIRED)
include_directories(vcpkg/installed/x64-windows/lib)
include_directories(vcpkg/installed/x64-windows/include)
# target_link_libraries(OpenGLProject PRIVATE CURL::libcurl)

# Add stb_image
target_include_directories(OpenGLProject PRIVATE src/stb_image.h)  # Adjust the path