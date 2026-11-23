/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Store.h"

#include <api/font/face/Face.h>

#include <api/font/glyph/colour/Glyph.h>
#include <api/font/glyph/colour/Fill.h>
#include <api/font/glyph/colour/Layer.h>
#include <api/font/glyph/colour/Stop.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "Glyph.h"
#include "Quadratic.h"
#include "Curves.h"

#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>
#include <glm/vec4.hpp>

namespace v3d::font::glyph::curve {

const Store::StoredCurves& Store::glyph(const face::Face& face, unsigned int index, const std::vector<std::int32_t>& coordinates) {
    auto key = std::make_tuple(&face, index, coordinates);
    const auto found = glyphs_.find(key);
    if (found != glyphs_.end()) {
        return found->second;
    }
    read_++;
    const Glyph outlined = outline(face, index, coordinates);
    StoredCurves& stored = glyphs_[std::move(key)];
    if (outlined.curves.empty()) {
        return stored;
    }
    stored = StoredCurves{ .bands = store(outlined), .min = outlined.min, .max = outlined.max, .empty = false };
    return stored;
}

std::uint32_t Store::store(const Glyph& outlined) {
    // the curves, two texels each, after those already stored
    const auto first = static_cast<std::uint32_t>(curveFloats_ / (std::size_t{2} * TEXEL));
    std::vector<float> curves;
    curves.reserve(outlined.curves.size() * (std::size_t{2} * TEXEL));
    for (const Quadratic& curve : outlined.curves) {
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
    return base;
}

const Store::StoredColour& Store::colour(const face::Face& face, unsigned int index, unsigned int palette, const std::vector<std::int32_t>& coordinates) {
    auto key = std::make_tuple(&face, index, palette, coordinates);
    const auto found = colours_.find(key);
    if (found != colours_.end()) {
        return found->second;
    }
    read_++;
    StoredColour& stored = colours_[std::move(key)];
    const colour::Glyph glyph = colour::read(face, index, palette, READ_SIZE, coordinates);
    // each fill's clips are stored first, so the program's own place is known when it is written
    std::vector<std::vector<float>> steps;
    std::vector<float> stops;
    for (const colour::Layer& layer : glyph.layers) {
        switch (layer.kind) {
        case colour::Layer::Kind::Fill:
            if (std::optional<std::vector<float>> step = fillStep(layer, &stops)) {
                steps.push_back(std::move(*step));
            }
            break;
        case colour::Layer::Kind::Image:
            stored.pictures = true;
            break;
        case colour::Layer::Kind::BeginGroup:
            steps.push_back(std::vector<float>(STEP, 0.0F));
            steps.back()[KIND] = BEGIN_GROUP;
            break;
        default:
            steps.push_back(std::vector<float>(STEP, 0.0F));
            steps.back()[KIND] = END_GROUP;
            steps.back()[COMPOSITE] = static_cast<float>(layer.composite);
            steps.back()[OPACITY] = layer.opacity;
            break;
        }
    }
    if (steps.empty() || glyph.empty()) {
        return stored;
    }

    const auto base = static_cast<std::uint32_t>(bandFloats_);
    const auto first = static_cast<float>(base + HEADER + (static_cast<std::uint32_t>(steps.size()) * STEP));
    std::vector<float> program{ static_cast<float>(steps.size()), 0.0F, 0.0F, 0.0F };
    for (std::vector<float>& step : steps) {
        if (step[KIND] == FILL) {
            step[FIRST_STOP] += first;
        }
        program.insert(program.end(), step.begin(), step.end());
    }
    program.insert(program.end(), stops.begin(), stops.end());
    append(&bands_, &bandFloats_, program, &bandsWritten_);
    stored.program = base;
    stored.min = glyph.min;
    stored.max = glyph.max;
    stored.empty = false;
    return stored;
}

std::optional<std::vector<float>> Store::fillStep(const colour::Layer& layer, std::vector<float>* stops) {
    std::vector<float> step(STEP, 0.0F);
    const auto clips = static_cast<std::uint32_t>(std::min<std::size_t>(layer.clips.size(), CLIPS));
    step[CLIP_COUNT] = static_cast<float>(clips);
    std::uint32_t evenOdd = 0;
    for (std::uint32_t clip = 0; clip < clips; clip++) {
        const Glyph outlined = banded(layer.clips[clip].curves);
        if (outlined.curves.empty()) {
            return std::nullopt;
        }
        step[FIRST_CLIP + clip] = static_cast<float>(store(outlined));
        evenOdd |= layer.clips[clip].evenOdd ? 1U << clip : 0U;
    }
    step[EVEN_ODD] = static_cast<float>(evenOdd);

    const colour::Fill& fill = layer.fill;
    if (glm::determinant(fill.transform) == 0.0F) {
        return std::nullopt;
    }
    const glm::mat3 back = glm::inverse(fill.transform);
    step[FILL_KIND] = static_cast<float>(fill.kind);
    step[EXTEND] = static_cast<float>(fill.extend);
    step[STOP_COUNT] = static_cast<float>(fill.stops.size());
    step[FIRST_STOP] = static_cast<float>(stops->size());
    const std::array<float, 12> geometry{ back[0][0], back[0][1], back[1][0], back[1][1], back[2][0], back[2][1], fill.from.x, fill.from.y,
        fill.to.x, fill.to.y, fill.kind == colour::Fill::Kind::Sweep ? fill.startAngle : fill.fromRadius,
        fill.kind == colour::Fill::Kind::Sweep ? fill.endAngle : fill.toRadius };
    std::copy(geometry.begin(), geometry.end(), step.begin() + GEOMETRY);
    for (const colour::Stop& stop : fill.stops) {
        stops->insert(stops->end(), { stop.offset, stop.colour.r, stop.colour.g, stop.colour.b, stop.colour.a, stop.foreground ? 1.0F : 0.0F });
    }
    return step;
}

const std::vector<float>& Store::curves() const noexcept {
    return curves_;
}

const std::vector<float>& Store::bands() const noexcept {
    return bands_;
}

std::uint32_t Store::curveRows() const noexcept {
    return static_cast<std::uint32_t>(curves_.size() / (static_cast<std::size_t>(ROW) * TEXEL));
}

std::uint32_t Store::bandRows() const noexcept {
    return static_cast<std::uint32_t>(bands_.size() / (static_cast<std::size_t>(ROW) * TEXEL));
}

Store::Changed Store::changedCurves() {
    return take(&curvesWritten_);
}

Store::Changed Store::changedBands() {
    return take(&bandsWritten_);
}

std::size_t Store::read() const noexcept {
    return read_;
}

std::size_t Store::append(std::vector<float>* array, std::size_t* used, const std::vector<float>& values, Written* written) {
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

Store::Changed Store::take(Written* written) {
    if (written->to <= written->from) {
        return Changed();
    }
    const std::size_t row = static_cast<std::size_t>(ROW) * TEXEL;
    const auto first = static_cast<std::uint32_t>(written->from / row);
    const auto last = static_cast<std::uint32_t>((written->to - 1) / row);
    *written = Written();
    return Changed{ .first = first, .rows = last - first + 1 };
}

};  // namespace v3d::font::glyph::curve
