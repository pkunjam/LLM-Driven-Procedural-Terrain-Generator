// main.cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

// Include our modules
#include "CameraManager.h"
#include "Renderer.h"
#include "Terrain.h"
#include "ChatInterface.h"
#include "Input.h"

// Global pointer to the terrain instance (to allow external updates)
Terrain* gTerrain = nullptr;

// Global camera instance.
// Adjust the parameters (target, distance, yaw, pitch) as needed.
ArcballCamera* gCamera = new ArcballCamera(glm::vec3(0.0f, 0.5f, 0.0f), 2.0f, -90.0f, -20.0f);

// These external functions are used by the ChatInterface module to update/revert terrain.
// (They simply call methods on our global Terrain instance.)
void updateTerrainExternally(int numOctaves, float persistence, float lacunarity, float baseAmplitude, float baseFrequency)
{
    if (gTerrain)
    {
        // In this example we keep the terrain dimensions fixed (500×500).
        gTerrain->updateTerrain(500, 500, numOctaves, persistence, lacunarity, baseAmplitude, baseFrequency);
    }
}

void undoTerrainChangeExternally()
{
    // Here you could restore a previous state (e.g. via a terrain history stack).
    // For this example, we simply print a message.
    std::cout << "Undo terrain change not implemented" << std::endl;
}

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Set GLFW window hints for OpenGL version and core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Create the GLFW window
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "LLM-Driven Terrain Generator", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    // Load OpenGL functions via GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // Set the viewport
    glViewport(0, 0, 1920, 1080);
    
    // Create module instances
    Renderer renderer;
    Terrain terrain;
    ChatInterface chatInterface;
    
    // Save our terrain pointer globally so that ChatInterface can update it.
    gTerrain = &terrain;
    
    // Register input callbacks (implemented in Input.cpp)
    glfwSetCursorPosCallback(window, Input::mouse_callback);
    glfwSetMouseButtonCallback(window, Input::mouse_button_callback);
    glfwSetScrollCallback(window, Input::scroll_callback);
    
    // Initialize modules
    renderer.initialize();
    terrain.initialize();
    chatInterface.initialize(window);
    
    // Main render loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        // Process input (only when ImGui is not capturing keyboard/mouse)
        Input::processInput(window);
        
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Render the scene (terrain + skybox)
        renderer.render(terrain);
        
        // Render the chat interface (ImGui overlay)
        chatInterface.render();
        
        // Swap buffers
        glfwSwapBuffers(window);
        
        // Check for OpenGL errors (for debugging)
        renderer.checkOpenGLError();
    }
    
    // Cleanup
    chatInterface.cleanup();
    renderer.cleanup();
    terrain.cleanup();
    glfwTerminate();
    delete gCamera;
    return 0;
}
