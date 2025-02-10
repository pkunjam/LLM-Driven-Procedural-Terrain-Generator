// Renderer.cpp
#include "Renderer.h"
#include "CameraManager.h" // To access gCamera
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "ArcballCamera.h"
#include "Terrain.h"
#include <iostream>
#include <stb_image.h>

// Utility function: compile a shader and check for errors.
static unsigned int compileShader(const char* shaderSource, GLenum shaderType)
{
    unsigned int shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSource, NULL);
    glCompileShader(shader);
    // Check for compilation errors.
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

Renderer::Renderer()
    : terrainShaderProgram(0), skyboxShaderProgram(0), waterShaderProgram(0),
      grassTexture(0), rockTexture(0), snowTexture(0), cubemapTexture(0),
      skyboxVAO(0), skyboxVBO(0),
      lightPos(0.0f, 2.0f, 5.0f)
{
}

Renderer::~Renderer()
{
}

void Renderer::initialize()
{
    glEnable(GL_DEPTH_TEST);
    
    // Create the terrain shader program.
    terrainShaderProgram = createTerrainShaderProgram();
    
    // Load textures for terrain.
    grassTexture = loadTexture("../resources/textures/grass.png");
    rockTexture  = loadTexture("../resources/textures/rock.png");
    snowTexture  = loadTexture("../resources/textures/snow.jpg");
    
    // Create the skybox shader program.
    skyboxShaderProgram = createSkyboxShaderProgram();
    
    // Setup skybox VAO and VBO.
    float skyboxVertices[] = {
        // positions
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };
    
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    
    // Load the cubemap texture for the skybox.
    std::vector<std::string> faces = {
        "../resources/textures/right.jpg",
        "../resources/textures/left.jpg",
        "../resources/textures/top.jpg",
        "../resources/textures/bottom.jpg",
        "../resources/textures/front.jpg",
        "../resources/textures/back.jpg"
    };
    cubemapTexture = loadCubemap(faces);
}

void Renderer::render(const Terrain& terrain)
{
    // Use the global camera for view and projection matrices.
    glm::mat4 view = gCamera->GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1920.0f/1080.0f, 0.1f, 100.0f);
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 mvp = projection * view * model;
    
    glUseProgram(terrainShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(terrainShaderProgram, "transform"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(glGetUniformLocation(terrainShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(terrainShaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(terrainShaderProgram, "viewPos"), 1, glm::value_ptr(gCamera->GetCameraPosition()));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, grassTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, rockTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, snowTexture);
    
    terrain.draw();
    
    // Render the skybox.
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyboxShaderProgram);
    
    glm::mat4 skyboxView = glm::mat4(glm::mat3(gCamera->GetViewMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(skyboxView));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}

void Renderer::cleanup()
{
    glDeleteProgram(terrainShaderProgram);
    glDeleteProgram(skyboxShaderProgram);
    // Delete textures.
    glDeleteTextures(1, &grassTexture);
    glDeleteTextures(1, &rockTexture);
    glDeleteTextures(1, &snowTexture);
    glDeleteTextures(1, &cubemapTexture);
    // Delete skybox buffers.
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
}

void Renderer::checkOpenGLError()
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error: " << err << std::endl;
    }
}

unsigned int Renderer::createTerrainShaderProgram()
{
    // Vertex shader source.
    const char* vertexShaderSource = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aTexCoord;
        layout(location = 2) in vec3 aNormal;
        
        out vec2 TexCoords;
        out vec3 FragPos;
        out vec3 Normal;
        
        uniform mat4 transform;
        uniform mat4 model;
        
        void main()
        {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal  = mat3(transpose(inverse(model))) * aNormal;
            TexCoords = aTexCoord;
            gl_Position = transform * vec4(aPos, 1.0);
        }
    )glsl";
    
    // Fragment shader source.
    const char* fragmentShaderSource = R"glsl(
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
        
        void main()
        {
            vec3 ambientLight = vec3(0.3);
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            vec3 viewDir  = normalize(viewPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * vec3(1.0);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = 0.5 * spec * vec3(1.0);
            vec3 lighting = ambientLight + diffuse + specular;
            
            vec4 grassColor = texture(grassTexture, TexCoords);
            vec4 rockColor  = texture(rockTexture, TexCoords);
            vec4 snowColor  = texture(snowTexture, TexCoords);
            vec4 baseColor;
            if (FragPos.y < 0.3)
                baseColor = grassColor;
            else if (FragPos.y < 0.6)
                baseColor = mix(grassColor, rockColor, (FragPos.y - 0.3) / 0.3);
            else
                baseColor = mix(rockColor, snowColor, (FragPos.y - 0.6) / 0.4);
            
            FragColor = vec4(lighting, 1.0) * baseColor;
        }
    )glsl";
    
    unsigned int vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // Check for linking errors.
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

unsigned int Renderer::createSkyboxShaderProgram()
{
    const char* vertexShaderSource = R"glsl(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        out vec3 TexCoords;
        uniform mat4 view;
        uniform mat4 projection;
        void main()
        {
            TexCoords = aPos;
            vec4 pos = projection * view * vec4(aPos, 1.0);
            gl_Position = pos.xyww; // Ensure the skybox is drawn behind everything
        }
    )glsl";
    
    const char* fragmentShaderSource = R"glsl(
        #version 330 core
        in vec3 TexCoords;
        out vec4 FragColor;
        uniform samplerCube skybox;
        void main()
        {
            FragColor = texture(skybox, TexCoords);
        }
    )glsl";
    
    unsigned int vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SKYBOX::SHADER::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

unsigned int Renderer::createWaterShaderProgram()
{
    // (Optional: implement water shader here if needed.)
    return 0;
}

unsigned int Renderer::loadTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
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
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    
    return textureID;
}

unsigned int Renderer::loadCubemap(const std::vector<std::string>& faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    
    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    return textureID;
}
