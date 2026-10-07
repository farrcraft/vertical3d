/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/vulkan/frame/Capture.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::test {

/**
 * Write back what a capture read out of a target, and compare it against the picture
 * committed under the same name.
 *
 * A reference may contain only output the Vulkan specification fixes exactly, so any
 * conformant driver produces it bit for bit. That means axis aligned geometry on integer pixel
 * boundaries, channel values of 0 or 1 or texels copied one per pixel, and no blend other than
 * an opaque one. A case that draws anything else checks individual pixels and an empty
 * validation log instead, and has no committed picture.
 *
 * The comparison is exact, so a case that breaks the rule fails on any device other than the
 * one that produced its reference.
 *
 * What was drawn is written to `data_out/<name>.png` whether or not it matched, because the
 * capture has no other way out. A deliberate change to a picture is that file copied over
 * the committed one by hand; nothing here writes `data/`.
 *
 * @param capture a capture whose readback has been submitted and waited on
 * @param name the picture's name, with no directory and no extension
 **/
void checkReference(const boost::shared_ptr<v3d::log::Logger>& logger,
    v3d::render::realtime::vulkan::frame::Capture* capture, const std::string& name);

};  // namespace v3d::test
