/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/log/Logger.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/kernel/integrate/FirstHit.h>
#include <moya/libmoya/output/Channel.h>
#include <moya/libmoya/output/Output.h>
#include <moya/libmoya/scene/Scene.h>

#include <optional>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::output {

/**
 * What a raw channel's source name reads where its path meets the scene: `depth`, `position`,
 * `normal`, `st`, `instance` or `face` at the first hit, and `albedo` or `denoisingNormal`
 * for a denoiser. None for any other name.
 **/
std::optional<kernel::integrate::Reading> readingNamed(const std::string & name);

/**
 * Every channel a scene's outputs name, each once, in the order the outputs name them. A channel
 * no output names is not rendered.
 *
 * A channel's filter names its combine: `mean`, `sum`, `closest`, `min` or `max`. Its
 * default is the closest for a raw depth, position, instance or face, and the mean for any other.
 **/
std::vector<Channel> channelsOf(const scene::Scene & scene);

/** The film channels of channelsOf(), the channels a film is made with. **/
std::vector<film::Channel> filmChannelsOf(const scene::Scene & scene);

/** The scene's outputs, in the order the scene holds them. **/
std::vector<Output> outputsOf(const scene::Scene & scene);

/** The format a file's name gives: PNG for a name ending in .png in any case, else EXR. **/
Format formatOfFile(const std::string & file);

/**
 * Writes an output from a snapshot. An EXR file holds the beauty and the output's channels, in
 * one part of its precision and compression. A PNG file holds the beauty through the
 * configuration's default display and view. A framebuffer writes nothing, which is logged.
 *
 * @throws std::invalid_argument when the output names a channel the snapshot lacks
 * @throws std::runtime_error when a PNG cannot be encoded or written
 * @throws std::exception, from OpenEXR, when an EXR file cannot be written
 **/
void write(const Output & output, const film::Snapshot & snapshot, const colour::Config & config, const boost::shared_ptr<log::Logger> & logger);

};  // namespace v3d::moya::output
