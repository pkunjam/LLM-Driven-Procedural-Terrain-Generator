// Input.cpp
#include "Input.h"
#include "imgui.h"
#include "CameraManager.h" // Include the header that declares gCamera
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// Global variables to track mouse state and interaction mode.
static bool leftMousePressed = false;
static bool orbitMode = false;   // No modifier: orbit the camera.
static bool panMode   = false;   // Shift pressed: pan the view.
static bool zoomMode  = false;   // Control pressed: zoom the view.
static float lastX = 400.0f, lastY = 300.0f;

void Input::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            leftMousePressed = true;
            if (mods & GLFW_MOD_SHIFT)
            {
                panMode = true;
                orbitMode = zoomMode = false;
            }
            else if (mods & GLFW_MOD_CONTROL)
            {
                zoomMode = true;
                orbitMode = panMode = false;
            }
            else
            {
                orbitMode = true;
                panMode = zoomMode = false;
            }
        }
        else if (action == GLFW_RELEASE)
        {
            leftMousePressed = false;
            orbitMode = panMode = zoomMode = false;
        }
    }
}

void Input::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    float xOffset = static_cast<float>(xpos - lastX);
    float yOffset = static_cast<float>(lastY - ypos); // Invert Y
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    if (leftMousePressed)
    {
        if (orbitMode)
            gCamera->ProcessMouseMovement(xOffset, yOffset, true);
        else if (panMode)
            gCamera->ProcessMousePan(xOffset, yOffset);
        else if (zoomMode)
            gCamera->ProcessMouseScroll(yOffset * 0.05f);
    }
}

void Input::scroll_callback(GLFWwindow* window, double xOffset, double yOffset)
{
    gCamera->ProcessMouseScroll(static_cast<float>(yOffset));
}

void Input::processInput(GLFWwindow* window)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse || io.WantCaptureKeyboard)
        return;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Optional keyboard panning
    glm::vec3 cameraFront = gCamera->GetCameraFront();
    glm::vec3 right = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f,1.0f,0.0f)));
    float cameraSpeed = 0.05f;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        gCamera->Target += cameraFront * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        gCamera->Target -= cameraFront * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        gCamera->Target -= right * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        gCamera->Target += right * cameraSpeed;
}
