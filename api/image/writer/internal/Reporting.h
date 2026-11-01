/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

namespace v3d::image::writer::internal {

/**
 * What libpng's error and warning handlers report through, the log.
 **/
struct Reporting {
    v3d::log::Logger* logger = nullptr;
};

};  // namespace v3d::image::writer::internal
