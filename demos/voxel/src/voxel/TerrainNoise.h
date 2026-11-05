/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include <glm/vec2.hpp>

/**
 * The fractal perlin noise behind a TerrainMap, sampled once into a 256x256 grid of heights.
 **/
class TerrainNoise final {
 public:
    void generate();
    /**
     * @param column the column's position in the grid
     * @return the column's height, a whole number from zero to 255, or zero outside the grid
     **/
    float height(const glm::uvec2& column);

 private:
    /**
     * The number of samples along each side of the grid.
     **/
    static constexpr unsigned int SIZE = 256;
    /**
     * The height of a column where the noise is at its highest.
     **/
    static constexpr float MAX_HEIGHT = 255.0F;
    /**
     * How far the grid reaches into the noise on each axis, in the noise's own units. More of the
     * noise in the same 256 samples makes the hills smaller and closer together.
     **/
    static constexpr float NOISE_EXTENT = 4.0F;
    /**
     * The number of octaves summed. Each has twice the frequency and half the amplitude of the one
     * before it.
     **/
    static constexpr int OCTAVES = 6;

    std::vector<float> heights_;
};
