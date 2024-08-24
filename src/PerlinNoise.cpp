#include <cmath>
#include <vector>

// Permutation table (for gradient hashing)
static const int permutation[] = {
    151, 160, 137, 91, 90, 15,
    131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142,
    // ... (full permutation table with 512 elements)
};

class PerlinNoise
{
public:
    PerlinNoise()
    {
        // Initialize permutation table with doubled values for hashing
        p.resize(512);
        for (int i = 0; i < 256; ++i)
            p[256 + i] = p[i] = permutation[i];
    }

    float noise(float x, float y)
    {
        // Find unit grid cell containing point
        int X = (int)std::floor(x) & 255;
        int Y = (int)std::floor(y) & 255;

        // Relative x, y positions in grid cell
        x -= std::floor(x);
        y -= std::floor(y);

        // Compute fade curves for x, y
        float u = fade(x);
        float v = fade(y);

        // Hash coordinates of the 4 cell corners
        int aa = p[p[X] + Y];
        int ab = p[p[X] + Y + 1];
        int ba = p[p[X + 1] + Y];
        int bb = p[p[X + 1] + Y + 1];

        // Blend results from the four corners of the grid
        float res = lerp(v, lerp(u, grad(aa, x, y), grad(ba, x - 1, y)),
                         lerp(u, grad(ab, x, y - 1), grad(bb, x - 1, y - 1)));
        return (res + 1.0) / 2.0; // Normalize to [0, 1]
    }

private:
    std::vector<int> p;

    // Fade function to smooth the interpolation
    float fade(float t)
    {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    // Linear interpolation
    float lerp(float t, float a, float b)
    {
        return a + t * (b - a);
    }

    // Gradient function to calculate dot product
    float grad(int hash, float x, float y)
    {
        int h = hash & 3;
        float u = h < 2 ? x : y;
        float v = h < 2 ? y : x;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
};
