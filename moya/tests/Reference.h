/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/image/Image.h>
#include <api/render/offline/film/FrameBuffer.h>
#include <moya/libmoya/state/RenderContext.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::reference {

/*
 * Helpers for the cases that compare a picture with a committed one, and for the cases that read
 * a scene through the old core.
 */

/** One sample at each pixel centre under a one pixel box. **/
void pixelCentres(v3d::moya::state::RenderContext & rc);

/**
 * Write what a case rendered to a file under data_out, so a failure can be looked at.
 **/
void keep(const boost::shared_ptr<v3d::image::Image> & rendered, const std::string & output);

/**
 * A rendered image against a committed one, writing what it rendered when they differ.
 **/
void check(const boost::shared_ptr<v3d::image::Image> & rendered, const char* reference,
    const char* output);

/**
 * A scene file, the hider to render it under, or null for the one the file names, and whether
 * it takes one sample at each pixel's centre, whatever sampling the file names.
 **/
struct Scene {
    const char* file = nullptr;
    const char* hider = nullptr;
    bool centres = false;
};

/**
 * A frame read from a file, under the hider the file names unless the caller names one first.
 **/
boost::shared_ptr<v3d::render::offline::film::FrameBuffer> read(const Scene & scene);

};  // namespace v3d::moya::reference
