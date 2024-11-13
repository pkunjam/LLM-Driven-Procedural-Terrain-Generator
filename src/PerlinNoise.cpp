#include <cmath>
#include <vector>
#include <random>
#include <algorithm>

class PerlinNoise {
public:
    PerlinNoise(unsigned int seed = 123) {
        // Initialize permutation vector with new seeding capability
        p.resize(512);
        std::iota(p.begin(), p.begin() + 256, 0); // Fill with 0..255
        
        // Seed the random generator
        std::default_random_engine engine(seed);
        std::shuffle(p.begin(), p.begin() + 256, engine);
        
        // Duplicate the permutation vector
        for (int i = 0; i < 256; ++i) {
            p[256 + i] = p[i];
        }
    }

    // Classic Perlin noise (2D)
    float noise(float x, float y, int octaves, float persistence) {
        float total = 0.0f;
        float maxValue = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;

        for (int i = 0; i < octaves; ++i) {
            total += singleNoise(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0f;
        }

        return total / maxValue;
    }

    // 3D Perlin noise
    float noise(float x, float y, float z, int octaves, float persistence) {
        float total = 0.0f;
        float maxValue = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;

        for (int i = 0; i < octaves; ++i) {
            total += singleNoise3D(x * frequency, y * frequency, z * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0f;
        }

        return total / maxValue;
    }

    // Ridged noise for mountain ranges
    float ridgedNoise(float x, float y, int octaves, float persistence) {
        float total = 0.0f;
        float maxValue = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;
        float weight = 1.0f;

        for (int i = 0; i < octaves; ++i) {
            float signal = singleNoise(x * frequency, y * frequency);
            signal = 2.0f * std::abs(signal - 0.5f); // Transform to create ridges
            signal = pow(1.0f - signal, 2.0f);       // Make ridges sharper with quadratic falloff
            
            // Weight successive octaves by previous signal
            signal *= weight;
            weight = signal * 2.0f;
            weight = std::clamp(weight, 0.0f, 1.0f);

            total += signal * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0f;
        }

        return total / maxValue;
    }

    // Billow noise for cloud-like formations
    float billow(float x, float y, int octaves, float persistence) {
        float total = 0.0f;
        float maxValue = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;

        for (int i = 0; i < octaves; ++i) {
            float signal = singleNoise(x * frequency, y * frequency);
            signal = 2.0f * std::abs(signal - 0.5f); // Create billowy appearance
            
            total += signal * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0f;
        }

        return total / maxValue;
    }

    // Voronoi (cellular) noise
    float voronoi(float x, float y, float frequency) {
        x *= frequency;
        y *= frequency;
        
        int xi = (int)std::floor(x);
        int yi = (int)std::floor(y);
        
        float minDist = 1000.0f;
        
        // Check surrounding cells
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                int cell_x = xi + i;
                int cell_y = yi + j;
                
                // Generate a pseudo-random point in the cell
                float point_x = cell_x + hash2D(cell_x, cell_y) / 255.0f;
                float point_y = cell_y + hash2D(cell_y, cell_x) / 255.0f;
                
                float dx = x - point_x;
                float dy = y - point_y;
                float dist = std::sqrt(dx * dx + dy * dy);
                
                minDist = std::min(minDist, dist);
            }
        }
        
        return minDist;
    }

private:
    std::vector<int> p;
    static const int permutation[256];

    float fade(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    float lerp(float t, float a, float b) {
        return a + t * (b - a);
    }

    float grad(int hash, float x, float y) {
        int h = hash & 3;
        float u = h < 2 ? x : y;
        float v = h < 2 ? y : x;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    // 3D gradient function
    float grad3D(int hash, float x, float y, float z) {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    float singleNoise(float x, float y) {
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

        float res = lerp(v,
            lerp(u, grad(aa, x, y), grad(ba, x - 1, y)),
            lerp(u, grad(ab, x, y - 1), grad(bb, x - 1, y - 1)));

        return (res + 1.0f) / 2.0f;
    }

    float singleNoise3D(float x, float y, float z) {
        int X = (int)std::floor(x) & 255;
        int Y = (int)std::floor(y) & 255;
        int Z = (int)std::floor(z) & 255;

        x -= std::floor(x);
        y -= std::floor(y);
        z -= std::floor(z);

        float u = fade(x);
        float v = fade(y);
        float w = fade(z);

        int A  = p[X] + Y;
        int AA = p[A] + Z;
        int AB = p[A + 1] + Z;
        int B  = p[X + 1] + Y;
        int BA = p[B] + Z;
        int BB = p[B + 1] + Z;

        float res = lerp(w, lerp(v, lerp(u, grad3D(p[AA], x, y, z),
                                          grad3D(p[BA], x-1, y, z)),
                                   lerp(u, grad3D(p[AB], x, y-1, z),
                                          grad3D(p[BB], x-1, y-1, z))),
                           lerp(v, lerp(u, grad3D(p[AA+1], x, y, z-1),
                                          grad3D(p[BA+1], x-1, y, z-1)),
                                   lerp(u, grad3D(p[AB+1], x, y-1, z-1),
                                          grad3D(p[BB+1], x-1, y-1, z-1))));
        
        return (res + 1.0f) / 2.0f;
    }

    // Hash function for Voronoi noise
    int hash2D(int x, int y) {
        int hash = x + y * 131;
        hash = (hash << 13) ^ hash;
        return (hash * (hash * hash * 15731 + 789221) + 1376312589) & 0x7fffffff;
    }
};

// The permutation array must be initialized
const int PerlinNoise::permutation[256] = {
    151, 160, 137, 91, 90, 15,
    131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142,
    8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26,
    197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149,
    56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48,
    27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105,
    92, 41, 55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216,
    80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188,
    159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
    5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16,
    58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154,
    163, 70, 221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98,
    108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228, 251, 34,
    242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14,
    239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121,
    50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72,
    243, 141, 128, 195, 78, 66, 215, 61, 156, 180
};
