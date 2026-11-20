/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/type/camera/Profile.h>

#include <cstdint>

namespace v3d::render::realtime::lighting {

/**
 * How fitCascades() fits a shadow map's cascades.
 **/
struct CascadeFit final {
    /** each layer's width in texels, at least 4 **/
    uint32_t texels = 2048;
    /**
     * The part of each cascade's far distance over which it blends into the next. The next
     * cascade starts that much nearer, so both cover the band. Held between 0 and 0.5
     **/
    float blend = 0.1F;
    /** the hand of the camera the scene is drawn through, as shadowLight() takes it **/
    type::camera::Profile::Hand hand = type::camera::Profile::Hand::UpCrossDirection;
};

};  // namespace v3d::render::realtime::lighting
