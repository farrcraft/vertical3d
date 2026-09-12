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
 * What a reference is allowed to contain is
 * [ADR-0054](../../../../docs/adr/0054-a-realtime-reference-is-a-picture-the-spec-determines.md).
 * A case drawing anything outside that rule asserts spot checks and validation silence
 * instead of calling this, and has no committed picture at all.
 *
 * The comparison is exact, so a case that drifts outside the rule fails on the device that
 * did not bless it rather than passing everywhere by a tolerance.
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
