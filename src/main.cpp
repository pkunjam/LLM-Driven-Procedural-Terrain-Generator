#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include "PerlinNoise.cpp"
#include "ArcballCamera.cpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Global Variables
ArcballCamera camera(glm::vec3(0.0f, 0.5f, 0.0f), 2.0f, -90.0f, -20.0f);
PerlinNoise perlin;
bool leftMousePressed = false;
bool rightMousePressed = false;
float lastX = 400.0f, lastY = 300.0f;

// Function Prototypes
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void scroll_callback(GLFWwindow *window, double xOffset, double yOffset);
void generateAdvancedTerrain(int width, int height, std::vector<float> &vertices, std::vector<unsigned int> &indices);
unsigned int createShaderProgram();
unsigned int loadTexture(const char *path);
void setupBuffers(unsigned int &VAO, unsigned int &VBO, unsigned int &EBO, const std::vector<float> &vertices, const std::vector<unsigned int> &indices);
void checkOpenGLError();

// Main Function
int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // GLFW Configuration
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a GLFW Window
    GLFWwindow *window = glfwCreateWindow(800, 600, "Advanced Perlin Noise Terrain Grid", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Set the viewport
    glViewport(0, 0, 800, 600);

    // Register Input Callbacks
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Configure Global OpenGL State
    glEnable(GL_DEPTH_TEST);

    // Load Textures
    unsigned int grassTexture = loadTexture("textures/grass.png");
    unsigned int rockTexture = loadTexture("textures/rock.png");
    unsigned int snowTexture = loadTexture("textures/snow.jpg");

    // Generate Advanced Terrain Grid
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateAdvancedTerrain(100, 100, vertices, indices);

    // Create Shader Program
    unsigned int shaderProgram = createShaderProgram();

    // Set texture uniforms
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "grassTexture"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "rockTexture"), 1);
    glUniform1i(glGetUniformLocation(shaderProgram, "snowTexture"), 2);

    // Setup Buffers
    unsigned int VAO, VBO, EBO;
    setupBuffers(VAO, VBO, EBO, vertices, indices);

    // Main Render Loop
    while (!glfwWindowShouldClose(window))
    {
        // Process Input
        processInput(window);

        // Clear Screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use Shader Program
        glUseProgram(shaderProgram);

        // Bind Textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grassTexture);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, rockTexture);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, snowTexture);

        // View/Projection Transformations
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 mvp = projection * view * model;

        // Send MVP Matrix to Shader
        int mvpLoc = glGetUniformLocation(shaderProgram, "transform");
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

        // Bind VAO and Draw the Grid
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // Swap Buffers and Poll Events
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Check for OpenGL errors
        checkOpenGLError();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    // Terminate GLFW
    glfwTerminate();
    return 0;
}

// Process Input
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// Mouse Movement Callback
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    if (leftMousePressed)
    {
        camera.ProcessMouseMovement(xOffset, yOffset, true);
    }
    if (rightMousePressed)
    {
        camera.ProcessMousePan(xOffset, yOffset);
    }
}

// Mouse Button Callback
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            leftMousePressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            leftMousePressed = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            rightMousePressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            rightMousePressed = false;
        }
    }
}

// Scroll Callback
void scroll_callback(GLFWwindow *window, double xOffset, double yOffset)
{
    camera.ProcessMouseScroll(yOffset);
}

// Generate Advanced Terrain with Multiple Layers of Perlin Noise
void generateAdvancedTerrain(int width, int height, std::vector<float> &vertices, std::vector<unsigned int> &indices)
{
    float scale = 1.0f / (std::max(width, height) - 1);

    int numOctaves = 4;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
    float baseAmplitude = 0.5f;
    float baseFrequency = 0.4f;

    for (int z = 0; z < height; ++z)
    {
        for (int x = 0; x < width; ++x)
        {
            float xPos = (x * scale) - 0.5f;
            float zPos = (z * scale) - 0.5f;

            float heightValue = 0.0f;
            float amplitude = baseAmplitude;
            float frequency = baseFrequency;

            for (int octave = 0; octave < numOctaves; ++octave)
            {
                heightValue += amplitude * perlin.noise(xPos * frequency, zPos * frequency, numOctaves, persistence);
                amplitude *= persistence;
                frequency *= lacunarity;
            }

            vertices.push_back(xPos);
            vertices.push_back(heightValue); // Height
            vertices.push_back(zPos);

            vertices.push_back(static_cast<float>(x) / (width - 1));  // Texture X coordinate
            vertices.push_back(static_cast<float>(z) / (height - 1)); // Texture Y coordinate

            if (x < width - 1 && z < height - 1)
            {
                int start = z * width + x;
                indices.push_back(start);
                indices.push_back(start + width);
                indices.push_back(start + 1);
                indices.push_back(start + 1);
                indices.push_back(start + width);
                indices.push_back(start + width + 1);
            }
        }
    }
}

// Create Shader Program
unsigned int createShaderProgram()
{
    const char *vertexShaderSource = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aTexCoord;
        out vec2 TexCoords;
        out float Height;

        uniform mat4 transform;

        void main() {
            TexCoords = aTexCoord;
            Height = aPos.y;
            gl_Position = transform * vec4(aPos, 1.0);
        }
    )glsl";

    const char *fragmentShaderSource = R"glsl(
        #version 330 core
        in vec2 TexCoords;
        in float Height;
        out vec4 FragColor;

        uniform sampler2D grassTexture;
        uniform sampler2D rockTexture;
        uniform sampler2D snowTexture;

        void main() {
            vec4 grassColor = texture(grassTexture, TexCoords);
            vec4 rockColor = texture(rockTexture, TexCoords);
            vec4 snowColor = texture(snowTexture, TexCoords);

            if (Height < 0.3)
                FragColor = grassColor;
            else if (Height < 0.6)
                FragColor = mix(grassColor, rockColor, (Height - 0.3) / 0.3);
            else
                FragColor = mix(rockColor, snowColor, (Height - 0.6) / 0.4);
        }
    )glsl";

    // Compile Vertex Shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Check for Vertex Shader compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    // Compile Fragment Shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Check for Fragment Shader compilation errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    // Link Shaders to Create the Shader Program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for Linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }

    // Clean up Shaders as they're now linked into the program
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Load Texture
unsigned int loadTexture(const char *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
        return 0;
    }

    return textureID;
}

// Setup Buffers
void setupBuffers(unsigned int &VAO, unsigned int &VBO, unsigned int &EBO, const std::vector<float> &vertices, const std::vector<unsigned int> &indices)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Check for OpenGL Errors
void checkOpenGLError()
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error: " << err << std::endl;
    }
}
