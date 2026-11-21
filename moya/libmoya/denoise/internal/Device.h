/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <OpenImageDenoise/oidn.hpp>

namespace v3d::moya::denoise::internal {

/** OIDN's CPU device, behind denoise::Denoiser. **/
struct Device {
    oidn::DeviceRef device;
};

};  // namespace v3d::moya::denoise::internal
