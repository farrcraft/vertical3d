/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CurvePages.h"

#include <api/font/glyph/CurveStore.h>
#include <api/render/realtime/resource/Handle.h>

#include <cstddef>
#include <cstdint>
#include <utility>

#include <glm/vec2.hpp>

namespace v3d::ui::paint {

/**
 **/
CurvePages::CurvePages(Create create, Update update, Release release) :
    create_(std::move(create)),
    update_(std::move(update)),
    release_(std::move(release)) {
}

/**
 **/
void CurvePages::sync(font::glyph::CurveStore* store) {
    sync(&curves_, store->curves().data(), store->curveRows(), store->changedCurves());
    sync(&bands_, store->bands().data(), store->bandRows(), store->changedBands());
}

/**
 **/
void CurvePages::sync(Page* page, const float* floats, std::uint32_t rows, const font::glyph::CurveStore::Changed& changed) {
    if (rows == 0) {
        return;
    }
    // an array that outgrew its texture is made again whole, with every row it holds
    if (rows > page->rows) {
        if (page->texture.valid()) {
            release_(page->texture);
        }
        page->texture = create_(floats, font::glyph::CurveStore::ROW, rows);
        page->rows = rows;
        return;
    }
    if (changed.rows > 0) {
        const std::size_t from = static_cast<std::size_t>(changed.first) * font::glyph::CurveStore::ROW * 4;
        update_(page->texture, glm::uvec2(0, changed.first), floats + from, glm::uvec2(font::glyph::CurveStore::ROW, changed.rows));
    }
}

/**
 **/
render::realtime::resource::TextureHandle CurvePages::curves() const noexcept {
    return curves_.texture;
}

/**
 **/
render::realtime::resource::TextureHandle CurvePages::bands() const noexcept {
    return bands_.texture;
}

};  // namespace v3d::ui::paint
