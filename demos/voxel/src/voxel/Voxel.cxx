/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#include "Voxel.h"

Voxel::Voxel(BlockType type, const glm::vec3 & position) :
    type_(type),
    position_(position),
    active_(true) {
    if (type_ == BLOCK_TYPE_AIR) {
        active_ = false;
    }
}

bool Voxel::active() const {
    return active_;
}

void Voxel::active(bool enabled) {
    active_ = enabled;
}

glm::vec3 Voxel::position() const {
    return position_;
}

Voxel::BlockType Voxel::type() const {
    return type_;
}
