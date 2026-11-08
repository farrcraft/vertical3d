/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/film/Film.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <glm/mat4x4.hpp>

/**
 * The files the new core writes its pictures to.
 **/
namespace v3d::moya::output {

/** The name the beauty goes by among a film's outputs, beside its channels' names. **/
inline constexpr std::string_view kBeauty = "beauty";

/** How each value of a part is stored: 16-bit or 32-bit floats. **/
enum class Precision : std::uint8_t {
    Half,
    Float
};

/** OpenEXR's compressions, from none to the lossy DWA and HTJ2K ones. **/
enum class Compression : std::uint8_t {
    None,
    Rle,
    Zips,
    Zip,
    Piz,
    Pxr24,
    B44,
    B44a,
    Dwaa,
    Dwab,
    Htj2k256,
    Htj2k32
};

/** One part of an EXR file: some of a film's outputs, stored one way. **/
struct ExrPart {
    /** The part's name. A file of several parts names each, and no two alike. **/
    std::string name;

    /** The outputs it holds, in order: kBeauty, or the name of one of the film's channels. **/
    std::vector<std::string> outputs;

    Precision precision { Precision::Half };
    Compression compression { Compression::Zip };
};

/** What an EXR file's header says about how its picture was made. **/
struct ExrMetadata {
    /** World to camera space, written as OpenEXR's worldToCamera. **/
    std::optional<glm::mat4> worldToCamera;

    /** World to screen space, written as OpenEXR's worldToNDC. **/
    std::optional<glm::mat4> worldToScreen;

    /** How long the picture took to render, written as moya/renderTime. **/
    std::optional<float> renderSeconds;
};

/**
 * Writes a snapshot of a film to an EXR file, one part for each part given, every part with the
 * whole of the snapshot's resolution. A single part is written as a plain scanline file, which
 * every EXR reader opens. Several are written as a multi-part file.
 *
 * The beauty is the channels R, G and B. A channel of one float is named as the channel is. A
 * channel of several floats is named as the channel is, followed by .R, .G, .B and .A in order,
 * as albedo.R, albedo.G and albedo.B. Every part carries ACEScg's chromaticities, the space the
 * film holds its beauty in, and the metadata given.
 *
 * @param path the file, as UTF-8
 * @throws std::invalid_argument for parts that cannot be written: none at all, a part with no
 *         outputs or one output twice, or an output the snapshot does not hold. Several parts
 *         each need a name, and no two the same. Nothing is written then
 * @throws std::exception, from OpenEXR, when the file cannot be written
 **/
void writeExr(const std::string & path, const film::Snapshot & snapshot, const std::vector<ExrPart> & parts,
    const ExrMetadata & metadata);

};  // namespace v3d::moya::output
