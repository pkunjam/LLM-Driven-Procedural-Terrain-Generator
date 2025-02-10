// Input.h
#ifndef INPUT_H
#define INPUT_H

#include <GLFW/glfw3.h>

class Input
{
public:
    // Processes per–frame keyboard input.
    static void processInput(GLFWwindow* window);
    
    // GLFW callback functions.
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void scroll_callback(GLFWwindow* window, double xOffset, double yOffset);
};

#endif // INPUT_H
