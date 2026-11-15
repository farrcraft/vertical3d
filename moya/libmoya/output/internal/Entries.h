/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <OpenEXR/ImfDeepFrameBuffer.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfPixelType.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace v3d::moya::output::internal {

/**
 * The entries of one of a checkpoint's deep parts, as OpenEXR reads and writes it, each field a
 * channel of unsigned integers. It holds each pixel's count of entries, and each field's values
 * for every entry in pixel order. It also holds where each pixel's first value of each field
 * lies.
 **/
struct Entries {
    std::vector<std::string> channels;
    std::vector<unsigned int> counts;
    std::vector<std::vector<std::uint32_t>> fields;
    std::vector<std::vector<char *>> pointers;

    /** Entries of the fields the channels name, in order, and no pixels yet. **/
    explicit Entries(std::vector<std::string> names) : channels(std::move(names)), fields(channels.size()), pointers(channels.size()) {}

    /** Points each pixel's place in each field's values at its first entry. **/
    void place() {
        for (std::size_t field = 0; field < fields.size(); ++field) {
            pointers[field].assign(counts.size(), nullptr);
            std::size_t first = 0;
            for (std::size_t pixel = 0; pixel < counts.size(); ++pixel) {
                pointers[field][pixel] = reinterpret_cast<char *>(fields[field].data() + first);
                first += counts[pixel];
            }
        }
    }

    /**
     * The frame a deep part reads its entries from or into, for a data window whose first pixel
     * lies origin pixels from the first held, width pixels across.
     **/
    Imf::DeepFrameBuffer frame(std::ptrdiff_t origin, std::size_t width) {
        Imf::DeepFrameBuffer out;
        out.insertSampleCountSlice(Imf::Slice(Imf::UINT, reinterpret_cast<char *>(counts.data() + origin), sizeof(unsigned int), sizeof(unsigned int) * width));
        for (std::size_t field = 0; field < fields.size(); ++field) {
            out.insert(channels[field],
                Imf::DeepSlice(Imf::UINT, reinterpret_cast<char *>(pointers[field].data() + origin), sizeof(char *), sizeof(char *) * width, sizeof(std::uint32_t)));
        }
        return out;
    }
};

};  // namespace v3d::moya::output::internal
