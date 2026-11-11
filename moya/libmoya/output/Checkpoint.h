/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/State.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace v3d::moya::output {

/** A checkpoint that cannot be resumed from: of another scene or film, or not a checkpoint. **/
class Refused : public std::runtime_error {
 public:
    using std::runtime_error::runtime_error;
};

/**
 * Writes a film's state to a checkpoint: an EXR file of one part, of floats, compressed without
 * loss, that any EXR reader opens. Every float a pixel holds is a channel. The beauty's weighted
 * sums are R, G and B, and a channel's are named as writeExr() names them. The floats after them
 * are moya.half.R, moya.half.G, moya.half.B and moya.half.weight, moya.sampleCount,
 * moya.moment.Y2, moya.moment.Y and moya.moment.weight2, and moya.weight. Each pixel's stop flag
 * is moya.stopped, of unsigned integers. The header's moya/passes holds the count of passes, and
 * moya/fingerprint the fingerprint given, as sixteen hexadecimal digits.
 *
 * The file is written beside the path, with .partial added to its name, and then renamed over
 * the path, so a render stopped while it writes leaves the checkpoint before.
 *
 * @param path the file, as UTF-8
 * @throws std::invalid_argument when the state's floats or flags do not fill its resolution, or
 *         when two of its floats would take one name in the file
 * @throws std::exception, from OpenEXR or the file system, when the file cannot be written
 **/
void writeCheckpoint(const std::string & path, const film::State & state, std::uint64_t fingerprint);

/**
 * Reads a checkpoint writeCheckpoint() wrote, for a film of the channels given.
 *
 * @param path the file, as UTF-8
 * @throws Refused when the file is not a checkpoint, when its fingerprint is not the one given,
 *         or when it lacks a channel the film's floats need
 * @throws std::exception, from OpenEXR, when the file cannot be read
 **/
film::State readCheckpoint(const std::string & path, const std::vector<film::Channel> & channels, std::uint64_t fingerprint);

};  // namespace v3d::moya::output
