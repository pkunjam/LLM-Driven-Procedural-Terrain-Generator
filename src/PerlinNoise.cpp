#include <cmath>
#include <vector>

class PerlinNoise
{
public:
    PerlinNoise()
    {
        p.resize(512);
        for (int i = 0; i < 256; ++i)
            p[256 + i] = p[i] = permutation[i];
    }

    // Perlin noise function with octaves and persistence
    float noise(float x, float y, int octaves, float persistence)
    {
        float total = 0.0f;
        float maxValue = 0.0f; // Used for normalization
        float frequency = 1.0f;
        float amplitude = 1.0f;

        for (int i = 0; i < octaves; ++i)
        {
            total += singleNoise(x * frequency, y * frequency) * amplitude;

            maxValue += amplitude;

            amplitude *= persistence;
            frequency *= 2.0f; // Increase frequency for next octave
        }

        return total / maxValue; // Normalize to [0, 1]
    }

private:
    std::vector<int> p;

    static const int permutation[256];

    float fade(float t)
    {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    float lerp(float t, float a, float b)
    {
        return a + t * (b - a);
    }

    float grad(int hash, float x, float y)
    {
        int h = hash & 3;
        float u = h < 2 ? x : y;
        float v = h < 2 ? y : x;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    float singleNoise(float x, float y)
    {
        int X = (int)std::floor(x) & 255;
        int Y = (int)std::floor(y) & 255;

        x -= std::floor(x);
        y -= std::floor(y);

        float u = fade(x);
        float v = fade(y);

        int aa = p[p[X] + Y];
        int ab = p[p[X] + Y + 1];
        int ba = p[p[X + 1] + Y];
        int bb = p[p[X + 1] + Y + 1];

        float res = lerp(v, lerp(u, grad(aa, x, y), grad(ba, x - 1, y)),
                         lerp(u, grad(ab, x, y - 1), grad(bb, x - 1, y - 1)));
        return (res + 1.0f) / 2.0f; // Normalize to [0, 1]
    }
};

// The permutation array must be initialized
const int PerlinNoise::permutation[256] = {
    151, 160, 137, 91, 90, 15,
    131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142,
    8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26,
    // ... (continue with the full 256-element permutation)
};
