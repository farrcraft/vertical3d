/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "GlyphPages.h"

#include <api/font/glyph/Store.h>
#include <api/render/realtime/resource/Handle.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::ui::paint {

/**
 **/
GlyphPages::GlyphPages(Create create, Update update, Release release) :
    create_(std::move(create)),
    update_(std::move(update)),
    release_(std::move(release)) {
}

/**
 **/
void GlyphPages::sync(font::glyph::Store* store) {
    while (textures_.size() > store->pages()) {
        release_(textures_.back());
        textures_.pop_back();
    }
    // a page added is made whole, so the plots the store gives for it need no copy of their own
    const std::size_t kept = textures_.size();
    const std::uint32_t side = store->spec().pageSize;
    const std::uint32_t channels = store->spec().channels;
    for (std::size_t page = kept; page < store->pages(); page++) {
        textures_.push_back(create_(store->pixels(page).data(), side, channels));
    }
    for (const font::glyph::Region& region : store->regions()) {
        if (region.page >= kept) {
            continue;
        }
        const std::vector<unsigned char>& pixels = store->pixels(region.page);
        const std::size_t width = static_cast<std::size_t>(region.size.x) * channels;
        rows_.resize(width * region.size.y);
        for (std::uint32_t row = 0; row < region.size.y; row++) {
            const std::size_t from = ((static_cast<std::size_t>(region.position.y + row) * side) + region.position.x) * channels;
            std::copy_n(pixels.begin() + static_cast<std::ptrdiff_t>(from), width, rows_.begin() + static_cast<std::ptrdiff_t>(row * width));
        }
        update_(textures_[region.page], region.position, rows_.data(), region.size, channels);
    }
}

/**
 **/
render::realtime::resource::TextureHandle GlyphPages::texture(std::size_t page) const {
    return page < textures_.size() ? textures_[page] : render::realtime::resource::TextureHandle();
}

/**
 **/
std::size_t GlyphPages::pages() const noexcept {
    return textures_.size();
}

};  // namespace v3d::ui::paint
