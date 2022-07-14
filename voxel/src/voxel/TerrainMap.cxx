/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "TerrainMap.h"

#include <noise.h>
#include "../noise/noiseutils.h"

class TerrainMap::Noise {
 public:
        void generate();
        float height(unsigned int x, unsigned int y);

 private:
        noise::utils::Image image_;
};


void TerrainMap::Noise::generate() {
    noise::module::Perlin perlinModule;
    noise::utils::NoiseMap heightMap;
    noise::utils::NoiseMapBuilderPlane heightMapBuilder;

    heightMapBuilder.SetSourceModule(perlinModule);
    heightMapBuilder.SetDestNoiseMap(heightMap);

    heightMapBuilder.SetDestSize(256, 256);
    // lower x, upper x, lower z, upper z
    heightMapBuilder.SetBounds(0.0, 4.0, 0.0, 4.0);

    heightMapBuilder.Build();

    // render to image
    noise::utils::RendererImage renderer;

    renderer.SetSourceNoiseMap(heightMap);
    renderer.SetDestImage(image_);
    // resulting image is greyscale
    renderer.Render();
}


float TerrainMap::Noise::height(unsigned int x, unsigned int y) {
    noise::utils::Color color;
    color = image_.GetValue(x, y);

    // r,g,b should be all equal for a greyscale image so any one will work
    return static_cast<float>(color.blue);
}


void TerrainMap::generate() {
    noise_.reset(new Noise());
    noise_->generate();
}

float TerrainMap::height(unsigned int x, unsigned int z) {
    return noise_->height(x, z);
}
