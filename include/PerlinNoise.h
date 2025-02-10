#ifndef PERLINNOISE_H
#define PERLINNOISE_H

#include <vector>

class PerlinNoise
{
public:
    /**
     * @brief Constructs a PerlinNoise generator.
     *
     * Initializes the permutation vector used for noise calculation.
     */
    PerlinNoise();

    /**
     * @brief Computes Perlin noise at the given (x, y) coordinate using multiple octaves.
     *
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @param octaves The number of noise octaves to combine.
     * @param persistence The amplitude decay factor for successive octaves.
     * @return A noise value normalized to the range [0, 1].
     */
    float noise(float x, float y, int octaves, float persistence);

private:
    std::vector<int> p;  ///< Permutation vector

    static const int permutation[256];  ///< Base permutation array

    /**
     * @brief Fade function as defined by Ken Perlin.
     *
     * @param t The input value.
     * @return The faded value.
     */
    float fade(float t);

    /**
     * @brief Linear interpolation between a and b.
     *
     * @param t The interpolation factor.
     * @param a The start value.
     * @param b The end value.
     * @return The interpolated value.
     */
    float lerp(float t, float a, float b);

    /**
     * @brief Computes a gradient based on a hash and input coordinates.
     *
     * @param hash The hash value.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @return The dot product between a pseudorandom gradient vector and the (x, y) offset.
     */
    float grad(int hash, float x, float y);

    /**
     * @brief Computes the noise value for a single octave.
     *
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @return A noise value in the range [0, 1].
     */
    float singleNoise(float x, float y);
};

#endif // PERLINNOISE_H
