/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/output/ExrPart.h>

#include <string>

namespace v3d::moya::output {

/**
 * Writes a snapshot's deep samples to a deep scanline EXR file, each pixel's samples front to
 * back. Each sample is the channels R, G, B and A, its colour premultiplied by its alpha, at the
 * precision given, then Z and ZBack, its depth, as 32-bit floats. Its objectid and materialid are
 * unsigned integers, its object's and material's IDs, which the header's idmanifest names by
 * MurmurHash3. A deep file is compressed by none, RLE or ZIPS, and any other compression is
 * written as ZIPS.
 *
 * @param path the file, as UTF-8
 * @throws std::invalid_argument when the snapshot holds no deep samples for every pixel
 * @throws std::exception, from OpenEXR, when the file cannot be written
 **/
void writeDeepExr(const std::string & path, const film::Snapshot & snapshot, Precision precision, Compression compression);

};  // namespace v3d::moya::output
