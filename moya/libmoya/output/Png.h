/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/log/Logger.h>
#include <moya/libmoya/colour/Transform.h>
#include <moya/libmoya/film/Snapshot.h>

#include <cstdint>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::output {

/**
 * A snapshot's beauty as a display shows it: each pixel through the display transform, then as 8
 * bits a channel. A value is clamped to [0, 1] and rounded to the nearest of 256 steps. A NaN is
 * zero.
 *
 * @param display a transform from colour::Config::display(), from the film's ACEScg to the display
 * @return red, green and blue for each pixel, row by row from the top left
 **/
std::vector<std::uint8_t> displayed(const film::Snapshot & snapshot, const colour::Transform & display);

/**
 * A colour channel of a snapshot as a display shows it, as displayed() shows the beauty.
 *
 * @param offset where the channel's first float sits in a pixel, as film::Snapshot::offset() gives it
 * @return red, green and blue for each pixel, row by row from the top left
 **/
std::vector<std::uint8_t> displayed(const film::Snapshot & snapshot, std::uint32_t offset, const colour::Transform & display);

/**
 * A channel of unit directions as 8 bits a component, with no display transform: each component
 * from -1 to 1 is mapped to 0 to 255, so a direction along x is red. A NaN is the middle step.
 *
 * @param offset where the channel's first float sits in a pixel, as film::Snapshot::offset() gives it
 * @return three bytes for each pixel, row by row from the top left
 **/
std::vector<std::uint8_t> directions(const film::Snapshot & snapshot, std::uint32_t offset);

/**
 * Writes a snapshot's beauty through a display transform to a PNG file, as displayed() gives it.
 *
 * @param path the file, as UTF-8
 * @param logger where the encoder reports what it could not do
 * @throws std::runtime_error when the image cannot be encoded or the file cannot be written
 **/
void writePng(const std::string & path, const film::Snapshot & snapshot, const colour::Transform & display,
    const boost::shared_ptr<log::Logger> & logger);

};  // namespace v3d::moya::output
