/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TerrainNoise.h"

#include <demos/voxel/src/noise/noiseutils.h>

#include <noise.h>

void TerrainNoise::generate() {
    const noise::module::Perlin perlinModule;
    noise::utils::NoiseMap heightMap;
    noise::utils::NoiseMapBuilderPlane heightMapBuilder;

    heightMapBuilder.SetSourceModule(perlinModule);
    heightMapBuilder.SetDestNoiseMap(heightMap);

    heightMapBuilder.SetDestSize(256, 256);
    // lower x, upper x, lower z, upper z
    heightMapBuilder.SetBounds(0.0, NOISE_EXTENT, 0.0, NOISE_EXTENT);

    heightMapBuilder.Build();

    // render to image
    noise::utils::RendererImage renderer;

    renderer.SetSourceNoiseMap(heightMap);
    renderer.SetDestImage(image_);
    // resulting image is greyscale
    renderer.Render();
}

float TerrainNoise::height(const glm::uvec2& column) {
    const unsigned int x = column.x;
    const unsigned int y = column.y;
    noise::utils::Color color;
    color = image_.GetValue(static_cast<int>(x), static_cast<int>(y));

    // r,g,b should be all equal for a greyscale image so any one will work
    return static_cast<float>(color.blue);
}
