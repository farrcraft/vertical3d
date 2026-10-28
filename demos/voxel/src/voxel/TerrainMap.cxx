/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "TerrainMap.h"

#include <demos/voxel/src/noise/noiseutils.h>

#include <noise.h>

namespace {

/**
 * How far the height map reaches into the noise on each axis, in the noise's own units.
 * More of the noise in the same 256 samples makes the hills smaller and closer together.
 **/
constexpr double NOISE_EXTENT = 4.0;

};  // namespace

class TerrainMap::Noise {
 public:
        void generate();
        float height(const glm::uvec2& column);

 private:
        noise::utils::Image image_;
};


void TerrainMap::Noise::generate() {
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


float TerrainMap::Noise::height(const glm::uvec2& column) {
    const unsigned int x = column.x;
    const unsigned int y = column.y;
    noise::utils::Color color;
    color = image_.GetValue(static_cast<int>(x), static_cast<int>(y));

    // r,g,b should be all equal for a greyscale image so any one will work
    return static_cast<float>(color.blue);
}


void TerrainMap::generate() {
    noise_.reset(new Noise());
    noise_->generate();
}

float TerrainMap::height(const glm::uvec2& column) {
    return noise_->height(column);
}
