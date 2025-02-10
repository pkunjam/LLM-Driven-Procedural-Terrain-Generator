cmake_minimum_required(VERSION 3.10)
project(OpenGLProject)

set(CMAKE_CXX_STANDARD 17)

# Add glad as a library
add_library(glad src/glad.c)

find_package(glfw3 3.4 REQUIRED)

# Add all source files including our modules and stb_image implementation.
set(SOURCES
    src/main.cpp
    src/ArcballCamera.cpp
    src/ChatInterface.cpp
    src/Input.cpp
    src/Terrain.cpp
    src/Renderer.cpp
    src/stb_image.cpp          # Added to provide STB_IMAGE_IMPLEMENTATION
    # ImGui source files.
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    # ImGui backend files.
    libs/imgui/backends/imgui_impl_glfw.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp
)

add_executable(OpenGLProject ${SOURCES})

# Include directories for your project.
target_include_directories(OpenGLProject PRIVATE
    glfw/3.4/include
    glm/
    src/
    include/
    libs/imgui
    libs/imgui/backends
)

include_directories(${CMAKE_SOURCE_DIR}/glad/include)
include_directories(${CMAKE_SOURCE_DIR}/glfw/3.4/include)

# Link libraries.
target_link_libraries(OpenGLProject PRIVATE glad glfw)

# On macOS, link the required system frameworks.
if(APPLE)
    target_link_libraries(OpenGLProject PRIVATE
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
endif()

# Link CURL if required.
find_package(CURL REQUIRED)
target_link_libraries(OpenGLProject PRIVATE curl)
