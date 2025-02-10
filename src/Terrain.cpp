// Terrain.cpp
#include "Terrain.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "PerlinNoise.cpp"  // Assumes your PerlinNoise class is defined here.
#include <iostream>

// A (global) PerlinNoise instance; alternatively you could add it as a member.
static PerlinNoise perlin;

Terrain::Terrain()
    : VAO(0), VBO(0), EBO(0),
      width(500), height(500),
      numOctaves(4), persistence(0.5f), lacunarity(2.0f),
      baseAmplitude(0.5f), baseFrequency(0.4f)
{
}

Terrain::~Terrain()
{
    cleanup();
}

void Terrain::initialize()
{
    generateAdvancedTerrain();
    setupBuffers();
}

void Terrain::draw() const
{
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Terrain::cleanup()
{
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
}

void Terrain::updateTerrain(int newWidth, int newHeight, int newNumOctaves, float newPersistence, float newLacunarity, float newBaseAmplitude, float newBaseFrequency)
{
    // Update parameters.
    width = newWidth;
    height = newHeight;
    numOctaves = newNumOctaves;
    persistence = newPersistence;
    lacunarity = newLacunarity;
    baseAmplitude = newBaseAmplitude;
    baseFrequency = newBaseFrequency;
    
    // Regenerate data.
    vertices.clear();
    indices.clear();
    normals.clear();
    generateAdvancedTerrain();
    setupBuffers();
}

void Terrain::generateAdvancedTerrain()
{
    // Compute a scale so that x and z positions remain in [-0.5, 0.5].
    float scale = 2.0f / (std::max(width, height) - 1);
    
    // Clear normals and later vertex and index arrays.
    normals.resize(width * height * 3, 0.0f);
    vertices.clear();
    indices.clear();

    // Temporary storage for the generated heights.
    std::vector<float> heights;
    heights.reserve(width * height);

    // Generate vertices: x, y, z, and texture coordinates.
    for (int z = 0; z < height; ++z)
    {
        for (int x = 0; x < width; ++x)
        {
            // Calculate fixed x and z positions.
            float xPos = (x * scale) - 0.5f;
            float zPos = (z * scale) - 0.5f;

            // Generate height using Perlin noise.
            float heightValue = 0.0f;
            float amplitude = baseAmplitude;
            float frequency = baseFrequency;

            for (int octave = 0; octave < numOctaves; ++octave)
            {
                heightValue += amplitude * perlin.noise(xPos * frequency, zPos * frequency, numOctaves, persistence);
                amplitude *= persistence;
                frequency *= lacunarity;
            }
            heights.push_back(heightValue);

            // Push vertex data: position then texture coordinates.
            vertices.push_back(xPos);        // x position (static)
            vertices.push_back(heightValue);   // y position (will adjust later)
            vertices.push_back(zPos);        // z position (static)
            vertices.push_back(static_cast<float>(x) / (width - 1)); // u
            vertices.push_back(static_cast<float>(z) / (height - 1)); // v
        }
    }

    // Compute the average height.
    float sum = 0.0f;
    for (float h : heights)
        sum += h;
    float avgHeight = sum / static_cast<float>(heights.size());

    // Subtract the average from every vertex's y coordinate so that the terrain is centered.
    size_t vertexCount = vertices.size() / 5; // 5 floats per vertex (x, y, z, u, v)
    for (size_t i = 0; i < vertexCount; ++i)
    {
        vertices[5 * i + 1] -= avgHeight;
    }

    // Generate indices for the grid.
    for (int z = 0; z < height - 1; ++z)
    {
        for (int x = 0; x < width - 1; ++x)
        {
            int topLeft     = z * width + x;
            int topRight    = topLeft + 1;
            int bottomLeft  = (z + 1) * width + x;
            int bottomRight = bottomLeft + 1;

            // First triangle.
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Second triangle.
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    // Calculate normals by averaging the normals for each triangle.
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        unsigned int idx0 = indices[i];
        unsigned int idx1 = indices[i + 1];
        unsigned int idx2 = indices[i + 2];

        // Retrieve vertex positions.
        glm::vec3 v0(vertices[5 * idx0], vertices[5 * idx0 + 1], vertices[5 * idx0 + 2]);
        glm::vec3 v1(vertices[5 * idx1], vertices[5 * idx1 + 1], vertices[5 * idx1 + 2]);
        glm::vec3 v2(vertices[5 * idx2], vertices[5 * idx2 + 1], vertices[5 * idx2 + 2]);

        // Compute two edges.
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        // Compute the face normal.
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        // Accumulate the normals.
        normals[3 * idx0]     += normal.x;
        normals[3 * idx0 + 1] += normal.y;
        normals[3 * idx0 + 2] += normal.z;
        normals[3 * idx1]     += normal.x;
        normals[3 * idx1 + 1] += normal.y;
        normals[3 * idx1 + 2] += normal.z;
        normals[3 * idx2]     += normal.x;
        normals[3 * idx2 + 1] += normal.y;
        normals[3 * idx2 + 2] += normal.z;
    }

    // Normalize the normals for each vertex.
    for (int i = 0; i < width * height; ++i)
    {
        glm::vec3 norm(normals[3 * i], normals[3 * i + 1], normals[3 * i + 2]);
        norm = glm::normalize(norm);
        normals[3 * i]     = norm.x;
        normals[3 * i + 1] = norm.y;
        normals[3 * i + 2] = norm.z;
    }
}

void Terrain::setupBuffers()
{
    // Delete old buffers if any.
    if (VAO) { glDeleteVertexArrays(1, &VAO); }
    if (VBO) { glDeleteBuffers(1, &VBO); }
    if (EBO) { glDeleteBuffers(1, &EBO); }
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    // Create an interleaved array: position (3 floats), texcoords (2 floats), normals (3 floats).
    std::vector<float> interleavedData;
    size_t vertexCount = vertices.size() / 5;
    for (size_t i = 0; i < vertexCount; ++i)
    {
        // Position.
        interleavedData.push_back(vertices[5 * i]);
        interleavedData.push_back(vertices[5 * i + 1]);
        interleavedData.push_back(vertices[5 * i + 2]);
        // Texture coordinates.
        interleavedData.push_back(vertices[5 * i + 3]);
        interleavedData.push_back(vertices[5 * i + 4]);
        // Normals.
        interleavedData.push_back(normals[3 * i]);
        interleavedData.push_back(normals[3 * i + 1]);
        interleavedData.push_back(normals[3 * i + 2]);
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(float), &interleavedData[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    
    // Set attribute pointers.
    // Positions.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // TexCoords.
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Normals.
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}
