/**
 * Voxel Engine
 *
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#pragma once

#include <boost/shared_ptr.hpp>

class TerrainMap
{
    public:
        void generate();

        float height(unsigned int x, unsigned int z);

    private:
        // pimpl
        class Noise;
        boost::shared_ptr<Noise> noise_;
};
