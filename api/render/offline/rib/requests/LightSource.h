/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

namespace v3d::render::offline::rib::requests {

/**
 * RiLightSource and RiAreaLightSource.
 *
 * RIB 3.03 writes the handle as a sequence number and later RIB writes a string. The reader
 * accepts both and stores the handle as a string, so a renderer keys its lights on one type.
 **/
struct LightSource {
    std::string shader;  /**< the light shader's name **/
    std::string handle;  /**< what a later Illuminate names this light by **/
};

};  // namespace v3d::render::offline::rib::requests
