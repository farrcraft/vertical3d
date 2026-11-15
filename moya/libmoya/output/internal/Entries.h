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
#include <iterator>
#include <vector>

namespace v3d::moya::output::internal {

/**
 * The entries of a checkpoint's coverage part, as OpenEXR reads and writes a deep part. It holds
 * each pixel's count of entries, and each field's values for every entry in pixel order. It also
 * holds where each pixel's first value of each field lies.
 **/
struct Entries {
    /**
     * The part's channels, each of unsigned integers: an entry's layer, its ID, and the high and
     * low halves of its weight's bits.
     **/
    static constexpr const char * const kChannels[] = { "layer", "id", "weight.high", "weight.low" };
    static constexpr std::size_t kFields = std::size(kChannels);

    std::vector<unsigned int> counts;
    std::vector<std::uint32_t> fields[kFields];
    std::vector<char *> pointers[kFields];

    /** Points each pixel's place in each field's values at its first entry. **/
    void place() {
        for (std::size_t field = 0; field < kFields; ++field) {
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
        for (std::size_t field = 0; field < kFields; ++field) {
            out.insert(kChannels[field],
                Imf::DeepSlice(Imf::UINT, reinterpret_cast<char *>(pointers[field].data() + origin), sizeof(char *), sizeof(char *) * width, sizeof(std::uint32_t)));
        }
        return out;
    }
};

};  // namespace v3d::moya::output::internal
