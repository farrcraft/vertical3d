/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/cpu/Textures.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/kernel/volume/Volumes.h>

namespace v3d::moya::light {

/**
 * What a volume's program runs with on the host, to find what the volume emits: the surfaces,
 * the texture access and the volumes, as kernel code reads them in a CPU device's memory.
 **/
struct Shading {
    kernel::surface::Surfaces surfaces;
    device::cpu::Textures textures;
    kernel::volume::Volumes volumes;
};

};  // namespace v3d::moya::light
