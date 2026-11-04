/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#pragma once

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

class TerrainNoise;

/**
 * The world's heightmap - fractal perlin noise over a 256x256 plane, read back per column.
 *
 * height() is virtual so that a chunk can be built against a map that is not a perlin one.
 * A test of how a column of blocks scales against the world ceiling then does not have to
 * generate noise.
 **/
class TerrainMap {
 public:
    virtual ~TerrainMap() = default;

    void generate();

    /**
     * @return the height of a column, in the range zero to 255
     **/
    virtual float height(const glm::uvec2& column);

 private:
    boost::shared_ptr<TerrainNoise> noise_;
};
