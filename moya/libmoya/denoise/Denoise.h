/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/log/Logger.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/output/Channel.h>

#include <vector>

#include <boost/shared_ptr.hpp>

/** Denoising a finished picture through OIDN. Host code. **/
namespace v3d::moya::denoise {

/**
 * The share of a picture's luminance OIDN is given, as the input scale over the log mean of each
 * pixel's luminance, as a photographic key.
 **/
inline constexpr float kKey = 0.18F;

/**
 * The input scale OIDN takes for a picture of ACEScg floats, three to a pixel at a stride. It is
 * kKey over the exponent of the mean log luminance of the pixels that hold any light, summed in
 * pixel order, and one for a picture of no light.
 **/
float scaleOf(const std::vector<float> & values, std::size_t stride, std::size_t first, std::size_t pixels);

/**
 * Denoises a snapshot through OIDN's RT filter on its CPU device, into the denoised copies its
 * film holds: the beauty's when denoising it, and each denoised channel's. The albedo and the
 * normal that guide every copy are filtered first, and every copy is then filtered with cleanAux
 * set, of the input scale scaleOf() gives its own picture.
 *
 * A filter that fails is reported, and its copy holds the noisy picture.
 *
 * @param channels the channels the snapshot's film was made from, the guides among them
 **/
void denoise(film::Snapshot & snapshot, const std::vector<output::Channel> & channels, bool beauty, const boost::shared_ptr<log::Logger> & logger);

};  // namespace v3d::moya::denoise
