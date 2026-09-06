/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#pragma once

#include <boost/shared_ptr.hpp>

/**
 * The world's heightmap - libnoise perlin over a 256x256 plane, read back per column.
 *
 * height() is virtual so that a chunk can be built against a map that is not a perlin one:
 * a test wanting to know how a column of blocks scales against the world ceiling has no
 * business generating noise to find out.
 **/
class TerrainMap {
 public:
    virtual ~TerrainMap() = default;

    void generate();

    /**
     * @return the height of a column, in the range zero to 255
     **/
    virtual float height(unsigned int x, unsigned int z);

 private:
    // pimpl
    class Noise;
    boost::shared_ptr<Noise> noise_;
};
