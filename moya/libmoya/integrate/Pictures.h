/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/integrate/Picture.h>

#include <cstdint>
#include <functional>
#include <string>

namespace v3d::moya::integrate {

/** How a session makes pictures as it samples. **/
struct Pictures {
    /**
     * The display and its view a picture is converted for, or empty for the configuration's
     * defaults.
     **/
    std::string display;
    std::string view;

    /**
     * The least seconds from one picture to the next, but for the first of each version and the
     * last.
     **/
    double interval { 0.0 };

    /**
     * The most pixels along the longer side of the first coarse picture after an edit, or zero
     * for no coarse pictures. A film no longer than this has none.
     **/
    std::uint32_t coarsest { 64 };

    /** Takes each picture, on the thread that runs the session's passes. **/
    std::function<void(const Picture &)> shown;
};

};  // namespace v3d::moya::integrate
