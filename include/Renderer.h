// Renderer.h
#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Terrain.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void initialize();               // Sets up shaders, textures, and skybox buffers
    void render(const Terrain& terrain); // Renders the terrain and skybox
    void cleanup();
    void checkOpenGLError();

private:
    // Shader programs
    unsigned int terrainShaderProgram;
    unsigned int skyboxShaderProgram;
    unsigned int waterShaderProgram; // (Optional: not used in this example)

    // Texture IDs
    unsigned int grassTexture;
    unsigned int rockTexture;
    unsigned int snowTexture;
    unsigned int cubemapTexture;

    // Skybox buffers
    unsigned int skyboxVAO;
    unsigned int skyboxVBO;

    // Lighting position
    glm::vec3 lightPos;

    // Internal helper functions to create shaders and load textures
    unsigned int createTerrainShaderProgram();
    unsigned int createSkyboxShaderProgram();
    unsigned int createWaterShaderProgram();
    unsigned int loadTexture(const char* path);
    unsigned int loadCubemap(const std::vector<std::string>& faces);
};

#endif // RENDERER_H
