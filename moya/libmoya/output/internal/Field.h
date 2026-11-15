/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/output/internal/ExrWindows.h>

#include <Imath/ImathBox.h>
#include <OpenEXR/ImfDeepFrameBuffer.h>
#include <OpenEXR/ImfPixelType.h>

#include <cstddef>
#include <vector>

namespace v3d::moya::output::internal {

/**
 * A field's values for every sample in pixel order, and where each pixel's first lies, as a deep
 * slice reads them, laid out row by row from the data window given to slice().
 **/
template <typename T>
struct Field {
    std::vector<T> values;
    std::vector<char *> pointers;

    void place(const std::vector<unsigned int> & counts) {
        pointers.assign(counts.size(), nullptr);
        std::size_t first = 0;
        for (std::size_t pixel = 0; pixel < counts.size(); ++pixel) {
            pointers[pixel] = reinterpret_cast<char *>(values.data() + first);
            first += counts[pixel];
        }
    }

    Imf::DeepSlice slice(Imf::PixelType type, std::size_t width, const Imath::Box2i & data) {
        return Imf::DeepSlice(type, based(reinterpret_cast<char *>(pointers.data()), data, sizeof(char *), sizeof(char *) * width), sizeof(char *),
            sizeof(char *) * width, sizeof(T));
    }
};

};  // namespace v3d::moya::output::internal
