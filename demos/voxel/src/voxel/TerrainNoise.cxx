/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TerrainNoise.h"

#include <cmath>
#include <cstddef>

#include <glm/common.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/vec3.hpp>

void TerrainNoise::generate() {
    heights_.assign(static_cast<std::size_t>(SIZE) * SIZE, 0.0F);
    const float step = NOISE_EXTENT / static_cast<float>(SIZE);
    for (unsigned int y = 0; y < SIZE; ++y) {
        for (unsigned int x = 0; x < SIZE; ++x) {
            // the grid is a plane through three-dimensional noise, which varies less than the
            // two-dimensional kind and so stays inside the clamp more often
            const glm::vec3 point(static_cast<float>(x) * step, 0.0F, static_cast<float>(y) * step);
            float value = 0.0F;
            float amplitude = 1.0F;
            float frequency = 1.0F;
            for (int octave = 0; octave < OCTAVES; ++octave) {
                value += amplitude * glm::perlin(point * frequency);
                amplitude *= 0.5F;
                frequency *= 2.0F;
            }
            // the sum is about -1 to 1, and is clamped where it reaches past either end
            const float shade = glm::clamp((value + 1.0F) * 0.5F, 0.0F, 1.0F);
            heights_[(static_cast<std::size_t>(y) * SIZE) + x] = std::floor(shade * MAX_HEIGHT);
        }
    }
}

float TerrainNoise::height(const glm::uvec2& column) {
    if (column.x >= SIZE || column.y >= SIZE) {
        return 0.0F;
    }
    return heights_[(static_cast<std::size_t>(column.y) * SIZE) + column.x];
}
