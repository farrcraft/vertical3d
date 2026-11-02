/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CurveStore.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "Curves.h"
#include "Face.h"

#include <glm/vec4.hpp>

namespace v3d::font {

const StoredCurves& CurveStore::glyph(const Face& face, unsigned int index) {
    const auto key = std::make_pair(&face, index);
    const auto found = glyphs_.find(key);
    if (found != glyphs_.end()) {
        return found->second;
    }
    read_++;
    const CurveGlyph outlined = outline(face, index);
    StoredCurves& stored = glyphs_[key];
    if (outlined.curves.empty()) {
        return stored;
    }

    // the curves, two texels each, after those already stored
    const auto first = static_cast<std::uint32_t>(curveFloats_ / (std::size_t{2} * TEXEL));
    std::vector<float> curves;
    curves.reserve(outlined.curves.size() * (std::size_t{2} * TEXEL));
    for (const QuadraticCurve& curve : outlined.curves) {
        curves.insert(curves.end(), { curve.p1.x, curve.p1.y, curve.p2.x, curve.p2.y, curve.p3.x, curve.p3.y, 0.0F, 0.0F });
    }
    append(&curves_, &curveFloats_, curves, &curvesWritten_);

    // the header, then each band's count and where its list starts, then the lists
    const auto base = static_cast<std::uint32_t>(bandFloats_);
    const glm::vec4 transform = outlined.bandTransform();
    std::vector<float> values{ transform.x, transform.y, transform.z, transform.w,
        static_cast<float>(outlined.vertical.size()), static_cast<float>(outlined.horizontal.size()), 0.0F, 0.0F };
    const std::size_t headers = 2 * (outlined.horizontal.size() + outlined.vertical.size());
    std::uint32_t list = base + static_cast<std::uint32_t>(values.size() + headers);
    std::vector<float> lists;
    for (const std::vector<std::vector<std::uint32_t>>* axis : { &outlined.horizontal, &outlined.vertical }) {
        for (const std::vector<std::uint32_t>& band : *axis) {
            values.push_back(static_cast<float>(band.size()));
            values.push_back(static_cast<float>(list));
            for (const std::uint32_t curve : band) {
                lists.push_back(static_cast<float>(first + curve));
            }
            list += static_cast<std::uint32_t>(band.size());
        }
    }
    values.insert(values.end(), lists.begin(), lists.end());
    append(&bands_, &bandFloats_, values, &bandsWritten_);

    stored = StoredCurves{ .bands = base, .min = outlined.min, .max = outlined.max, .empty = false };
    return stored;
}

const std::vector<float>& CurveStore::curves() const noexcept {
    return curves_;
}

const std::vector<float>& CurveStore::bands() const noexcept {
    return bands_;
}

std::uint32_t CurveStore::curveRows() const noexcept {
    return static_cast<std::uint32_t>(curves_.size() / (static_cast<std::size_t>(ROW) * TEXEL));
}

std::uint32_t CurveStore::bandRows() const noexcept {
    return static_cast<std::uint32_t>(bands_.size() / (static_cast<std::size_t>(ROW) * TEXEL));
}

CurveStore::Changed CurveStore::changedCurves() {
    return take(&curvesWritten_);
}

CurveStore::Changed CurveStore::changedBands() {
    return take(&bandsWritten_);
}

std::size_t CurveStore::read() const noexcept {
    return read_;
}

std::size_t CurveStore::append(std::vector<float>* array, std::size_t* used, const std::vector<float>& values, Written* written) {
    const std::size_t start = *used;
    const std::size_t row = static_cast<std::size_t>(ROW) * TEXEL;
    std::size_t rows = std::max<std::size_t>(array->size() / row, 1);
    while (rows * row < start + values.size()) {
        rows *= 2;
    }
    if (array->size() < rows * row) {
        array->resize(rows * row, 0.0F);
    }
    std::copy(values.begin(), values.end(), array->begin() + static_cast<std::ptrdiff_t>(start));
    *used = start + values.size();
    written->from = written->to > written->from ? std::min(written->from, start) : start;
    written->to = std::max(written->to, *used);
    return start;
}

CurveStore::Changed CurveStore::take(Written* written) {
    if (written->to <= written->from) {
        return Changed();
    }
    const std::size_t row = static_cast<std::size_t>(ROW) * TEXEL;
    const auto first = static_cast<std::uint32_t>(written->from / row);
    const auto last = static_cast<std::uint32_t>((written->to - 1) / row);
    *written = Written();
    return Changed{ .first = first, .rows = last - first + 1 };
}

};  // namespace v3d::font
