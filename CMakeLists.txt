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

# set(CMAKE_PREFIX_PATH "vcpkg/installed/x64-windows/share/curl" ${CMAKE_PREFIX_PATH})
# find_package(CURL CONFIG REQUIRED)
# find_package(jsoncpp CONFIG REQUIRED)

# target_link_libraries(OpenGLProject PRIVATE CURL::libcurl jsoncpp_lib)

# find_package(ZLIB REQUIRED)
# target_link_libraries(OpenGLProject PRIVATE ZLIB::ZLIB)