/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "EffectQuad.h"

#include <api/font/Face.h>
#include <api/font/Field.h>
#include <api/font/GlyphStore.h>
#include <api/font/TextStyle.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/canvas/FieldRamp.h>
#include <api/ui/paint/GlyphPages.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::ui::paint::internal {

std::optional<EffectQuad> effectQuad(font::GlyphStore* store, const font::Face& face, unsigned int index, float size,
    const font::TextEffect& effect) {
    if (store == nullptr || !(size > 0.0F)) {
        return std::nullopt;
    }
    const font::GlyphKey key = font::fieldKey(&face, index, font::effectReach(effect, size));
    const std::optional<font::StoredGlyph> stored = store->glyph(key, font::field);
    if (!stored.has_value() || stored->size.x == 0 || stored->size.y == 0) {
        return std::nullopt;
    }
    const float scale = size / font::FIELD_SIZE;
    const auto side = static_cast<float>(store->spec().pageSize);
    const glm::vec2 min = (glm::vec2(stored->bearing) * scale) + effect.offset;
    const glm::vec2 ramp = font::effectRamp(effect, size, key.spread);
    return EffectQuad{ .min = min,
        .max = min + (glm::vec2(stored->size) * scale),
        .uv0 = glm::vec2(stored->position) / side,
        .uv1 = glm::vec2(stored->position + stored->size) / side,
        .page = stored->page,
        .ramp = render::realtime::canvas::fieldRamp(ramp.x, ramp.y) };
}

void placeEffects(std::vector<PlacedEffect>* placed, font::GlyphStore* store, const font::Face& face, unsigned int index, float size,
    const std::vector<font::TextEffect>& effects, const glm::vec2& pen) {
    for (std::size_t layer = 0; layer < effects.size(); layer++) {
        std::optional<EffectQuad> quad = effectQuad(store, face, index, size, effects[layer]);
        if (quad.has_value()) {
            quad->min += pen;
            quad->max += pen;
            placed->push_back(PlacedEffect{ .quad = *quad, .layer = layer, .colour = effects[layer].colour });
        }
    }
}

void layerEffects(std::vector<PlacedEffect>* placed) {
    std::stable_sort(placed->begin(), placed->end(), [](const PlacedEffect& left, const PlacedEffect& right) {
        return left.layer < right.layer;
    });
}

void drawEffects(render::realtime::canvas::Canvas* canvas, const std::vector<PlacedEffect>& placed, const GlyphPages& pages, const glm::mat3& transform,
    std::vector<render::realtime::canvas::Canvas::Vertex>* vertices, std::vector<std::uint32_t>* indices) {
    constexpr std::array<std::uint32_t, 6> triangles{ 0, 1, 2, 2, 3, 0 };
    vertices->clear();
    indices->clear();
    for (std::size_t index = 0; index < placed.size(); index++) {
        const PlacedEffect& effect = placed[index];
        const EffectQuad& quad = effect.quad;
        const std::array<glm::vec2, 4> corners{ quad.min, glm::vec2(quad.max.x, quad.min.y), quad.max, glm::vec2(quad.min.x, quad.max.y) };
        const std::array<glm::vec2, 4> uv{ quad.uv0, glm::vec2(quad.uv1.x, quad.uv0.y), quad.uv1, glm::vec2(quad.uv0.x, quad.uv1.y) };
        const auto first = static_cast<std::uint32_t>(vertices->size());
        for (std::size_t corner = 0; corner < corners.size(); corner++) {
            const glm::vec3 moved = transform * glm::vec3(corners[corner], 1.0F);
            vertices->push_back({ .position = glm::vec2(moved), .uv = uv[corner], .colour = effect.colour, .glyph = quad.ramp });
        }
        for (const std::uint32_t corner : triangles) {
            indices->push_back(first + corner);
        }
        if (index + 1 == placed.size() || placed[index + 1].quad.page != quad.page) {
            canvas->text(*vertices, *indices, pages.texture(quad.page));
            vertices->clear();
            indices->clear();
        }
    }
}

};  // namespace v3d::ui::paint::internal
