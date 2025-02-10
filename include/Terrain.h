// Terrain.h
#ifndef TERRAIN_H
#define TERRAIN_H

#include <vector>
#include <glad/glad.h>

class Terrain
{
public:
    Terrain();
    ~Terrain();

    // Initializes the terrain (generates the grid and creates the OpenGL buffers)
    void initialize();
    
    // Draws the terrain (binds its VAO and issues the draw call)
    void draw() const;
    
    // Releases OpenGL resources used by the terrain
    void cleanup();
    
    // Updates the terrain parameters and regenerates the grid.
    // In this example, the terrain dimensions remain fixed at 500×500.
    void updateTerrain(int width, int height, int numOctaves, float persistence, float lacunarity, float baseAmplitude, float baseFrequency);

private:
    // Terrain data arrays.
    std::vector<float> vertices;         // Each vertex: [x, y, z, u, v]
    std::vector<unsigned int> indices;   // Triangle indices
    std::vector<float> normals;          // Normals per vertex (3 floats per vertex)
    
    // OpenGL buffers.
    unsigned int VAO, VBO, EBO;
    
    // Terrain dimensions and parameters.
    int width;
    int height;
    int numOctaves;
    float persistence;
    float lacunarity;
    float baseAmplitude;
    float baseFrequency;
    
    // Internal helper functions.
    void generateAdvancedTerrain();
    void setupBuffers();
};

#endif // TERRAIN_H
