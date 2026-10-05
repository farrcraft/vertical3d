/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/memory/Image.h>

#include "Sampler.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::pipeline {

/**
 * An image the shaders sample from, and the sampler they read it through.
 *
 * Both are shared rather than owned. A texture memory::TextureFactory built is the only
 * holder of its image; one registered from a frame::RenderTarget shares the target's, so
 * the image outlives whichever of the two lets go first. Either way a released texture is
 * destroyed by the last reference going, which the ring holds until no frame can read it.
 **/
struct Texture final {
    boost::shared_ptr<memory::Image> image;
    boost::shared_ptr<Sampler> sampler;
};

};  // namespace v3d::render::realtime::vulkan::pipeline
