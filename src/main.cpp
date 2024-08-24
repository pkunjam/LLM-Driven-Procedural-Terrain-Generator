#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <PerlinNoise.cpp>

PerlinNoise perlin; // Create a Perlin noise object

// Vertex Shader source code
const char *vertexShaderSource = R"glsl(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    uniform mat4 transform;
    void main() {
        gl_Position = transform * vec4(aPos, 1.0);
    }
)glsl";

// Fragment Shader source code
const char *fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(0.53, 0.44, 0.26, 1.0);
    }
)glsl";

// Function to generate grid vertices and indices using advanced Perlin noise
void generateGrid(int width, int height, std::vector<float> &vertices, std::vector<unsigned int> &indices, int octaves, float persistence)
{
    float scale = 1.0f / (std::max(width, height) - 1); // Scale to fit in [-0.5, 0.5]
    for (int z = 0; z < height; ++z)
    {
        for (int x = 0; x < width; ++x)
        {
            float xPos = (x * scale) - 0.5f;
            float zPos = (z * scale) - 0.5f;
            float yPos = perlin.noise(xPos, zPos, octaves, persistence); // Use advanced Perlin noise

            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);

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

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Set up the GLFW window properties: OpenGL version 3.3, core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create the window
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

    // Compile Vertex Shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Compile Fragment Shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Link shaders to create the shader program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Delete the shaders as they’re linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Generate grid vertices and indices using advanced Perlin noise
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateGrid(100, 100, vertices, indices, 5, 0.5f); // Create a 100x100 grid with 5 octaves and 0.5 persistence

    // Set up vertex data and buffer(s) and configure vertex attributes
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Unbind VBO and VAO to avoid modifying them by mistake
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Set the clear color (background color)
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Main loop to render the scene
    while (!glfwWindowShouldClose(window))
    {
        // Clear the color buffer
        glClear(GL_COLOR_BUFFER_BIT);

        // Use the shader program
        glUseProgram(shaderProgram);

        // Create a transformation matrix
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::rotate(transform, (float)glfwGetTime() * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));

        // Get the matrix's uniform location and set its value
        int transformLoc = glGetUniformLocation(shaderProgram, "transform");
        transform = glm::translate(transform, glm::vec3(0.0f, -0.5f, 0.0f));
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

        // Bind the VAO
        glBindVertexArray(VAO);

        // Draw the grid
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // Swap the buffers to display the result
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Clean up and delete resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    // Clean up and exit
    glfwTerminate();
    return 0;
}
