/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/film/Snapshot.h>

#include <Imath/ImathBox.h>
#include <Imath/ImathVec.h>

#include <cstddef>
#include <cstdint>

namespace v3d::moya::output::internal {

/** An EXR file's two windows: the whole image, and the pixels the file holds. **/
struct Windows {
    Imath::Box2i display;
    Imath::Box2i data;
};

/**
 * The windows of a snapshot's file. The display window is the snapshot's image, or its own
 * resolution when it names none, and the data window is its pixels at their place in it.
 **/
inline Windows windowsOf(const film::Snapshot & snapshot) {
    const bool whole = snapshot.image.x == 0U || snapshot.image.y == 0U;
    const int imageWidth = static_cast<int>(whole ? snapshot.resolution.x : snapshot.image.x);
    const int imageHeight = static_cast<int>(whole ? snapshot.resolution.y : snapshot.image.y);
    const Imath::V2i origin(static_cast<int>(whole ? 0U : snapshot.origin.x), static_cast<int>(whole ? 0U : snapshot.origin.y));
    const Imath::V2i last = origin + Imath::V2i(static_cast<int>(snapshot.resolution.x) - 1, static_cast<int>(snapshot.resolution.y) - 1);
    return Windows { .display = Imath::Box2i(Imath::V2i(0, 0), Imath::V2i(imageWidth - 1, imageHeight - 1)), .data = Imath::Box2i(origin, last) };
}

/**
 * The base OpenEXR addresses a slice from, for pixels laid out row by row from the data window's
 * top left. OpenEXR finds a pixel at the base plus its place in the image times the strides, so
 * the base lies before the first pixel by the data window's corner.
 **/
inline char * based(char * first, const Imath::Box2i & data, std::size_t across, std::size_t down) {
    const std::size_t before = (static_cast<std::size_t>(data.min.x) * across) + (static_cast<std::size_t>(data.min.y) * down);
    // OpenEXR's own Slice::Make offsets its base by the same address arithmetic
    return reinterpret_cast<char *>(reinterpret_cast<std::uintptr_t>(first) - before);
}

};  // namespace v3d::moya::output::internal
