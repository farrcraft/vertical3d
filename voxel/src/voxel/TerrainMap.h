/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#pragma once

#include <boost/shared_ptr.hpp>

class TerrainMap {
 public:
    void generate();

    float height(unsigned int x, unsigned int z);

 private:
    // pimpl
    class Noise;
    boost::shared_ptr<Noise> noise_;
};
