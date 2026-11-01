/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <demos/voxel/src/noise/noiseutils.h>

#include <glm/vec2.hpp>

/**
 * The perlin noise behind a TerrainMap, rendered once to a 256x256 greyscale image. Only
 * TerrainMap's source includes this header, so only that source compiles against libnoise.
 **/
class TerrainNoise final {
 public:
    void generate();
    float height(const glm::uvec2& column);

 private:
    /**
     * How far the height map reaches into the noise on each axis, in the noise's own units.
     * More of the noise in the same 256 samples makes the hills smaller and closer together.
     **/
    static constexpr double NOISE_EXTENT = 4.0;

    noise::utils::Image image_;
};
