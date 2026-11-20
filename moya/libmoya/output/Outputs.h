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
#include <string_view>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::output {

/**
 * What a raw channel's source name reads where its path meets the scene. At the first hit, that is
 * `depth`, `position`, `normal`, `st`, `instance`, `face` or `motion`. For a denoiser, it
 * is `albedo` or `denoisingNormal`. None for any other name.
 **/
std::optional<kernel::integrate::Reading> readingNamed(const std::string & name);

/**
 * Every channel a scene's outputs name, each once, in the order the outputs name them. A channel
 * no output names is not rendered. A render that denoises anything ends with the albedo and the
 * normal that guide it, kDenoiseAlbedo and kDenoiseNormal, unless the scene names its own.
 *
 * A channel's filter names its combine: `mean`, `sum`, `closest`, `min` or `max`. Its
 * default is the closest for a raw depth, position, instance or face, and the mean for any other.
 **/
std::vector<Channel> channelsOf(const scene::Scene & scene);

/**
 * The film channels of channelsOf(), the channels a film is made with. A Cryptomatte layer is the
 * channels of its ranks, and a denoised channel is followed by its denoised copy, of three
 * components. A render that denoises its beauty ends with the denoised beauty's channel.
 **/
std::vector<film::Channel> filmChannelsOf(const scene::Scene & scene);

/** The names of the guides a render that denoises adds when the scene names none of its own. **/
inline constexpr std::string_view kDenoiseAlbedo = "denoiseAlbedo";
inline constexpr std::string_view kDenoiseNormal = "denoiseNormal";

/** What a denoised copy's name adds to the name of the channel it copies, or to the beauty's. **/
inline constexpr std::string_view kDenoised = "_denoised";

/** Whether the scene's options ask for the beauty denoised. **/
bool denoisesBeauty(const scene::Scene & scene);

/** The name of a channel's denoised copy. **/
std::string denoisedOf(const std::string & channel);

/**
 * The scene's outputs, in the order the scene holds them. An EXR output of a render that denoises
 * its beauty starts with the denoised beauty, and each denoised channel it names is followed by
 * its denoised copy.
 **/
std::vector<Output> outputsOf(const scene::Scene & scene);

/** The format a file's name gives: PNG for a name ending in .png in any case, else EXR. **/
Format formatOfFile(const std::string & file);

/**
 * A snapshot whose beauty, and its denoised beauty where it holds one, is composited over a colour
 * in ACEScg by the beauty's alpha, and whose alpha is then one. Every other channel, coverage and
 * deep sample is the snapshot's.
 **/
film::Snapshot composited(const film::Snapshot & snapshot, const glm::vec3 & background);

/**
 * Writes an output from a snapshot. An EXR file holds the beauty and the output's channels, in
 * one part of its precision and compression. A PNG file holds the beauty through the
 * configuration's default display and view. A framebuffer writes nothing, which is logged. An
 * output with a background writes the snapshot composited over it.
 *
 * @throws std::invalid_argument when the output names a channel the snapshot lacks
 * @throws std::runtime_error when a PNG cannot be encoded or written
 * @throws std::exception, from OpenEXR, when an EXR file cannot be written
 **/
void write(const Output & output, const film::Snapshot & snapshot, const colour::Config & config, const boost::shared_ptr<log::Logger> & logger);

};  // namespace v3d::moya::output
