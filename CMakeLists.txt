cmake_minimum_required(VERSION 3.10)
project(OpenGLProject)

set(CMAKE_CXX_STANDARD 17)

# Include GLFW
add_subdirectory(libs/GLFW)

# Include GLAD
add_library(glad libs/GLAD/src/glad.c)
target_include_directories(glad PUBLIC libs/GLAD/include)

# Add source files
set(SOURCES
    src/main.cpp
    # ImGui source files
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    # ImGui backend files
    libs/imgui/backends/imgui_impl_glfw.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp
)

add_executable(OpenGLProject ${SOURCES})

# Include directories
target_include_directories(OpenGLProject PRIVATE
    libs/GLFW/include
    libs/GLAD/include
    glm
    src/
    include
    libs/imgui
    libs/imgui/backends
)

# Link libraries
target_link_libraries(OpenGLProject PRIVATE glfw glad)

# Add stb_image
target_include_directories(OpenGLProject PRIVATE src)

# Macbook inclusion (if needed)
find_package(CURL REQUIRED)
target_link_libraries(OpenGLProject PRIVATE curl)
