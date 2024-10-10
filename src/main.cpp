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
#include "UserPrompt.h"

// Global variables for terrain parameters
int numOctaves = 4;
float persistence = 0.5f;
float lacunarity = 2.0f;
float baseAmplitude = 0.5f;
float baseFrequency = 0.4f;

float waterLevel = 0.5f;         // Initial water level
unsigned int waterVAO, waterVBO; // Water plane buffers

unsigned int VAO, VBO, EBO;

// Global Variables
ArcballCamera camera(glm::vec3(0.0f, 0.5f, 0.0f), 2.0f, -90.0f, 20.0f);
PerlinNoise perlin;
bool leftMousePressed = false;
bool rightMousePressed = false;
float lastX = 400.0f, lastY = 300.0f;

// Function Prototypes
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void scroll_callback(GLFWwindow *window, double xOffset, double yOffset);
void generateAdvancedTerrain(int width, int height, std::vector<float> &vertices, std::vector<unsigned int> &indices, std::vector<float> &normals);
unsigned int createShaderProgram();
unsigned int loadTexture(const char *path);
void setupBuffers(unsigned int &VAO, unsigned int &VBO, unsigned int &EBO, const std::vector<float> &vertices, const std::vector<float> &normals, const std::vector<unsigned int> &indices);
void checkOpenGLError();
void checkForUserPrompt(GLFWwindow* window);
unsigned int compileShader(const char *shaderSource, GLenum shaderType);

void generateWaterPlane(std::vector<float> &waterVertices, float width, float depth);
void setupWaterBuffers(unsigned int &waterVAO, unsigned int &waterVBO, const std::vector<float> &waterVertices);
unsigned int createWaterShaderProgram();

// lighting
glm::vec3 lightPos = glm::vec3(5.0f, 10.0f, 5.0f);

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

    std::cout << "Grass texture ID: " << grassTexture << std::endl;
    std::cout << "Rock texture ID: " << rockTexture << std::endl;
    std::cout << "Snow texture ID: " << snowTexture << std::endl;

    // Generate Advanced Terrain Grid
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    std::vector<float> normals;
    generateAdvancedTerrain(100, 100, vertices, indices, normals);

    std::cout << "Vertices generated: " << vertices.size() << std::endl;
    std::cout << "Normals generated: " << normals.size() << std::endl;
    std::cout << "Indices generated: " << indices.size() << std::endl;


    // Create Shader Program
    unsigned int shaderProgram = createShaderProgram();

    // Set texture uniforms
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "grassTexture"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "rockTexture"), 1);
    glUniform1i(glGetUniformLocation(shaderProgram, "snowTexture"), 2);

    // Setup Buffers
    setupBuffers(VAO, VBO, EBO, vertices, normals, indices);

    // Generate Water Plane
    std::vector<float> waterVertices;
    generateWaterPlane(waterVertices, 1.0f, 1.0f); // Adjust width and depth as needed

    // Create Water Shader Program
    unsigned int waterShaderProgram = createWaterShaderProgram();

    // Setup Water Buffers
    setupWaterBuffers(waterVAO, waterVBO, waterVertices);

    // Main Render Loop
    while (!glfwWindowShouldClose(window))
    {
        // Process Input
        processInput(window);

        // check if the 'P' key is pressed for user input
        checkForUserPrompt(window);

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

        // Pass Lighting Information to Shader
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(camera.GetCameraPosition()));

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

        // Render Water Plane
        glUseProgram(waterShaderProgram);

        // Set the transformation matrix for the water shader
        int waterTransformLoc = glGetUniformLocation(waterShaderProgram, "transform");
        glUniformMatrix4fv(waterTransformLoc, 1, GL_FALSE, glm::value_ptr(mvp));

        // Set the water color (RGBA)
        glUniform4f(glGetUniformLocation(waterShaderProgram, "waterColor"), 0.0f, 0.3f, 0.6f, 0.5f); // Adjust color and transparency as desired

        float timeValue = glfwGetTime(); // Get the elapsed time
        glUniform1f(glGetUniformLocation(waterShaderProgram, "time"), timeValue);

        glm::vec3 viewPosition = camera.GetCameraPosition();
        glUniform3fv(glGetUniformLocation(waterShaderProgram, "viewPos"), 1, glm::value_ptr(viewPosition));

        // Bind Water VAO and draw
        glBindVertexArray(waterVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

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

    // Delete water resources
    glDeleteVertexArrays(1, &waterVAO);
    glDeleteBuffers(1, &waterVBO);
    glDeleteProgram(waterShaderProgram);

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
void generateAdvancedTerrain(int width, int height, std::vector<float> &vertices, std::vector<unsigned int> &indices, std::vector<float> &normals)
{
    float scale = 1.0f / (std::max(width, height) - 1);

    // Clear previous normals
    normals.resize(width * height * 3, 0.0f); // x, y, z normals for each vertex

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

            // Texture coordinates
            vertices.push_back(static_cast<float>(x) / (width - 1));
            vertices.push_back(static_cast<float>(z) / (height - 1));

            // If not at the last row/column, generate indices for this quad
            if (x < width - 1 && z < height - 1)
            {
                int topLeft = z * width + x;
                int topRight = topLeft + 1;
                int bottomLeft = (z + 1) * width + x;
                int bottomRight = bottomLeft + 1;

                // Triangle 1
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                // Triangle 2
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }
    }

    // Calculate normals by averaging adjacent triangle normals
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        // Get vertex indices for this triangle
        unsigned int idx0 = indices[i];
        unsigned int idx1 = indices[i + 1];
        unsigned int idx2 = indices[i + 2];

        // Get vertex positions
        glm::vec3 v0(vertices[5 * idx0], vertices[5 * idx0 + 1], vertices[5 * idx0 + 2]);
        glm::vec3 v1(vertices[5 * idx1], vertices[5 * idx1 + 1], vertices[5 * idx1 + 2]);
        glm::vec3 v2(vertices[5 * idx2], vertices[5 * idx2 + 1], vertices[5 * idx2 + 2]);

        // Calculate two edges
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;

        // Calculate normal using cross product
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        // Add normal to each vertex of the triangle
        normals[3 * idx0] += normal.x;
        normals[3 * idx0 + 1] += normal.y;
        normals[3 * idx0 + 2] += normal.z;

        normals[3 * idx1] += normal.x;
        normals[3 * idx1 + 1] += normal.y;
        normals[3 * idx1 + 2] += normal.z;

        normals[3 * idx2] += normal.x;
        normals[3 * idx2 + 1] += normal.y;
        normals[3 * idx2 + 2] += normal.z;
    }

    // Normalize the normals for each vertex
    for (int i = 0; i < width * height; ++i)
    {
        glm::vec3 normal(normals[3 * i], normals[3 * i + 1], normals[3 * i + 2]);
        normal = glm::normalize(normal);
        normals[3 * i] = normal.x;
        normals[3 * i + 1] = normal.y;
        normals[3 * i + 2] = normal.z;
    }
}

void generateWaterPlane(std::vector<float> &waterVertices, float width, float depth)
{
    waterVertices = {
        -0.5f * width, waterLevel, -0.5f * depth,
        0.5f * width, waterLevel, -0.5f * depth,
        -0.5f * width, waterLevel, 0.5f * depth,

        0.5f * width, waterLevel, -0.5f * depth,
        0.5f * width, waterLevel, 0.5f * depth,
        -0.5f * width, waterLevel, 0.5f * depth};
}

unsigned int compileShader(const char *shaderSource, GLenum shaderType)
{
    unsigned int shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSource, NULL);
    glCompileShader(shader);

    // Check for compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    return shader;
}

// Create Shader Program
unsigned int createShaderProgram()
{
    const char *vertexShaderSource = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aTexCoord;
        layout(location = 2) in vec3 aNormal;

        out vec2 TexCoords;
        out vec3 FragPos;
        out vec3 Normal;

        uniform mat4 transform;

        void main() {
            TexCoords = aTexCoord;
            FragPos = vec3(transform * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(transform))) * aNormal; // Correct the normal based on transformations
            gl_Position = transform * vec4(aPos, 1.0);
        }

    )glsl";

    const char *fragmentShaderSource = R"glsl(
        
        #version 330 core
        in vec2 TexCoords;
        in vec3 FragPos;
        in vec3 Normal;

        out vec4 FragColor;

        uniform vec3 lightPos;
        uniform vec3 viewPos;

        uniform sampler2D grassTexture;
        uniform sampler2D rockTexture;
        uniform sampler2D snowTexture;

        void main() {
            // Ambient lighting
            float ambientStrength = 0.5;
            vec3 ambient = ambientStrength * vec3(1.0);

            // Diffuse lighting
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * vec3(1.0);

            // Specular lighting (reintroduce this now)
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); // Adjust shininess
            vec3 specular = specularStrength * spec * vec3(1.0);

            // Texture blending
            vec4 grassColor = texture(grassTexture, TexCoords);
            vec4 rockColor = texture(rockTexture, TexCoords);
            vec4 snowColor = texture(snowTexture, TexCoords);

            vec4 baseColor;
            if (FragPos.y < 0.3)
                baseColor = grassColor;
            else if (FragPos.y < 0.6)
                baseColor = mix(grassColor, rockColor, (FragPos.y - 0.3) / 0.3);
            else
                baseColor = mix(rockColor, snowColor, (FragPos.y - 0.6) / 0.4);

            // Apply ambient, diffuse, and specular lighting
            FragColor = vec4((ambient + diffuse + specular), 1.0) * baseColor;
         
        }
        
    )glsl";

    unsigned int vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);

    // Shader Program linking
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

unsigned int createWaterShaderProgram()
{
    const char *waterVertexShaderSource = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;

        uniform mat4 transform;
        uniform float time;  // Add time for dynamic water movement

        void main() {
            // Simulate water movement using a sine wave
            vec3 position = aPos;
            position.y += sin(position.x * 10.0 + time) * 0.05; // Adjust wave frequency and amplitude

            gl_Position = transform * vec4(position, 1.0);
        }
    )glsl";

    const char *waterFragmentShaderSource = R"glsl(
        #version 330 core
        out vec4 FragColor;

        uniform vec4 waterColor;
        uniform vec3 viewPos;  // Camera position for Fresnel effect

        void main() {
            // Fresnel effect based on the angle between view and surface normal
            float fresnel = dot(normalize(viewPos), vec3(0.0, 1.0, 0.0)); // Water is flat, so normal is (0, 1, 0)
            fresnel = clamp(1.0 - fresnel, 0.0, 1.0);  // Control strength of reflection

            // Final water color with Fresnel effect
            vec4 finalColor = mix(waterColor, vec4(1.0, 1.0, 1.0, 1.0), fresnel * 0.2);  // Blend with white to simulate reflection
            FragColor = finalColor;
        }
    )glsl";

    // Compile Vertex Shader
    unsigned int waterVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(waterVertexShader, 1, &waterVertexShaderSource, NULL);
    glCompileShader(waterVertexShader);

    // Check for Vertex Shader compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(waterVertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(waterVertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    // Compile Fragment Shader
    unsigned int waterFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(waterFragmentShader, 1, &waterFragmentShaderSource, NULL);
    glCompileShader(waterFragmentShader);

    // Check for Fragment Shader compilation errors
    glGetShaderiv(waterFragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(waterFragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    // Link Shaders to Create the Shader Program
    unsigned int waterShaderProgram = glCreateProgram();
    glAttachShader(waterShaderProgram, waterVertexShader);
    glAttachShader(waterShaderProgram, waterFragmentShader);
    glLinkProgram(waterShaderProgram);

    // Check for Linking errors
    glGetProgramiv(waterShaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(waterShaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }

    // Clean up Shaders as they're now linked into the program
    glDeleteShader(waterVertexShader);
    glDeleteShader(waterFragmentShader);

    return waterShaderProgram; // Return the water shader program ID
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
void setupBuffers(unsigned int &VAO, unsigned int &VBO, unsigned int &EBO, const std::vector<float> &vertices, const std::vector<float> &normals, const std::vector<unsigned int> &indices)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Create an interleaved vertex buffer (positions + normals + texture coordinates)
    std::vector<float> interleavedData;
    for (size_t i = 0; i < vertices.size() / 5; ++i)
    {
        interleavedData.push_back(vertices[5 * i]);     // x
        interleavedData.push_back(vertices[5 * i + 1]); // y
        interleavedData.push_back(vertices[5 * i + 2]); // z
        interleavedData.push_back(vertices[5 * i + 3]); // u (texture coordinate)
        interleavedData.push_back(vertices[5 * i + 4]); // v (texture coordinate)
        interleavedData.push_back(normals[3 * i]);      // normal x
        interleavedData.push_back(normals[3 * i + 1]);  // normal y
        interleavedData.push_back(normals[3 * i + 2]);  // normal z
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(float), &interleavedData[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // Set vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0); // Position
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float))); // TexCoords
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(5 * sizeof(float))); // Normals
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void setupWaterBuffers(unsigned int &waterVAO, unsigned int &waterVBO, const std::vector<float> &waterVertices)
{
    glGenVertexArrays(1, &waterVAO);
    glGenBuffers(1, &waterVBO);

    glBindVertexArray(waterVAO);

    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferData(GL_ARRAY_BUFFER, waterVertices.size() * sizeof(float), &waterVertices[0], GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

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

// Function to check for key press (for user prompt)
void checkForUserPrompt(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        std::string userPrompt = getUserPrompt();
        handleUserPrompt(userPrompt);
    }
}