/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "TerrainMap.h"

#include "TerrainNoise.h"

void TerrainMap::generate() {
    noise_.reset(new TerrainNoise());
    noise_->generate();
}

float TerrainMap::height(const glm::uvec2& column) {
    return noise_->height(column);
}
