/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/output/ExrMetadata.h>
#include <moya/libmoya/output/ExrPart.h>

#include <string>
#include <string_view>
#include <vector>

/**
 * The files the new core writes its pictures to.
 **/
namespace v3d::moya::output {

/** The name the beauty goes by among a film's outputs, beside its channels' names. **/
inline constexpr std::string_view kBeauty = "beauty";

/**
 * Writes a snapshot of a film to an EXR file, one part for each part given, every part with the
 * whole of the snapshot's resolution. A single part is written as a plain scanline file, which
 * every EXR reader opens. Several are written as a multi-part file.
 *
 * The beauty is the channels R, G and B. A channel of one float is named as the channel is. A
 * channel of several floats is named as the channel is, followed by .R, .G, .B and .A in order,
 * as albedo.R, albedo.G and albedo.B. Every part carries the metadata given. A colour space whose
 * ID is one of the linear ones with standard primaries, AP0, AP1, Rec. 709, P3-D65, Rec. 2020 or
 * Adobe RGB, also writes its chromaticities.
 *
 * @param path the file, as UTF-8
 * @throws std::invalid_argument for parts that cannot be written: none at all, a part with no
 *         outputs or one output twice, or an output the snapshot does not hold. It is thrown too
 *         for a part whose outputs give two channels one name in the file, as a channel named R
 *         beside the beauty does. Several parts each need a name, and no two the same. Nothing is
 *         written then
 * @throws std::exception, from OpenEXR, when the file cannot be written
 **/
void writeExr(const std::string & path, const film::Snapshot & snapshot, const std::vector<ExrPart> & parts,
    const ExrMetadata & metadata);

};  // namespace v3d::moya::output
