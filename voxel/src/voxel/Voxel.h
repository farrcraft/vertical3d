/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include <boost/shared_ptr.hpp>

/**
 * A single voxel
 */
class Voxel {
 public:
    typedef enum : std::uint8_t {
        BLOCK_TYPE_AIR,
        BLOCK_TYPE_DIRT,
        BLOCK_TYPE_GRASS,
        BLOCK_TYPE_SAND,
        BLOCK_TYPE_STONE,
        BLOCK_TYPE_GRAVEL,
        BLOCK_TYPE_WATER,
        BLOCK_TYPE_ORE,
        BLOCK_TYPE_WOOD,
        BLOCK_TYPE_LAVA,
        BLOCK_TYPE_GLASS,
        BLOCK_TYPE_BEDROCK,
        BLOCK_TYPE_CLAY,
        BLOCK_TYPE_ICE,
        BLOCK_TYPE_SANDSTONE,
        BLOCK_TYPE_OBSIDIAN,
        BLOCK_TYPE_SNOW
    } BlockType;

    typedef enum : std::uint16_t {
        BLOCK_FACE_FRONT = 2,
        BLOCK_FACE_LEFT = 4,
        BLOCK_FACE_RIGHT = 8,
        BLOCK_FACE_BACK = 16,
        BLOCK_FACE_TOP = 32,
        BLOCK_FACE_BOTTOM = 64,
        BLOCK_FACE_ALL = 128,
        BLOCK_FACE_NONE = 256
    } BlockFace;

    /**
        * A block of a type, at a position in world blocks.
        */
    Voxel(BlockType type, const glm::vec3 & position);

    bool active() const;
    glm::vec3 position() const;
    BlockType type() const;

 protected:
     void active(bool enabled);

 private:
    BlockType type_;
    glm::vec3 position_;
    bool active_;
};
