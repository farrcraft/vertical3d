/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "GlyphStore.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::font {

GlyphStore::GlyphStore() :
    GlyphStore(Spec()) {
}

GlyphStore::GlyphStore(const Spec& spec) :
    spec_(spec) {
    spec_.plotSize = std::clamp(spec_.plotSize, std::uint32_t{1}, std::max(spec_.pageSize, std::uint32_t{1}));
    spec_.pageSize = std::max(spec_.pageSize - (spec_.pageSize % spec_.plotSize), spec_.plotSize);
    spec_.channels = spec_.channels == 4 ? 4 : 1;
}

void GlyphStore::beginFrame() {
    // a page past the limit goes once the frame just ended used nothing in it
    const std::size_t perPage = static_cast<std::size_t>(spec_.pageSize / spec_.plotSize) * (spec_.pageSize / spec_.plotSize);
    while (pages_.size() > std::max<std::size_t>(spec_.pages, 1)) {
        const std::size_t first = (pages_.size() - 1) * perPage;
        const bool idle = std::all_of(plots_.begin() + static_cast<std::ptrdiff_t>(first), plots_.end(),
            [this](const Plot& plot) { return plot.used < frame_; });
        if (!idle) {
            break;
        }
        for (std::size_t plot = first; plot < plots_.size(); plot++) {
            empty(plot);
        }
        plots_.resize(first);
        pages_.pop_back();
    }
    frame_++;
}

std::optional<StoredGlyph> GlyphStore::glyph(const GlyphKey& key, const Rasterizer& rasterize) {
    const auto found = entries_.find(key);
    if (found != entries_.end()) {
        plots_[found->second.plot].used = frame_;
        return found->second.stored;
    }
    const GlyphBitmap bitmap = rasterize(key);
    made_++;
    if (bitmap.channels != spec_.channels || bitmap.pixels.size() != static_cast<std::size_t>(bitmap.size.x) * bitmap.size.y * bitmap.channels) {
        return std::nullopt;
    }
    const std::optional<std::pair<std::size_t, glm::uvec2>> spot = room(bitmap.size);
    if (!spot) {
        return std::nullopt;
    }
    Plot& plot = plots_[spot->first];
    const StoredGlyph stored{ .page = plot.page, .position = spot->second, .size = bitmap.size, .bearing = bitmap.bearing };
    blit(bitmap, plot.page, stored.position);
    plot.keys.push_back(key);
    plot.used = frame_;
    plot.dirty = true;
    entries_.emplace(key, Entry{ .stored = stored, .plot = spot->first });
    return stored;
}

std::size_t GlyphStore::pages() const noexcept {
    return pages_.size();
}

const std::vector<unsigned char>& GlyphStore::pixels(std::size_t page) const {
    return pages_.at(page);
}

std::vector<GlyphRegion> GlyphStore::regions() {
    std::vector<GlyphRegion> changed;
    for (Plot& plot : plots_) {
        if (plot.dirty) {
            changed.push_back(GlyphRegion{ .page = plot.page, .position = plot.origin, .size = glm::uvec2(spec_.plotSize) });
            plot.dirty = false;
        }
    }
    return changed;
}

std::size_t GlyphStore::made() const noexcept {
    return made_;
}

const GlyphStore::Spec& GlyphStore::spec() const noexcept {
    return spec_;
}

std::optional<glm::uvec2> GlyphStore::place(Plot* plot, const glm::uvec2& size) const {
    // a glyph with no pixels takes no room
    if (size.x == 0 || size.y == 0) {
        return plot->origin;
    }
    // the padding goes to the right of and below each glyph, and the plot's first row and
    // column are padding too, so no two glyphs touch
    const std::uint32_t padding = spec_.padding;
    const std::uint32_t plotSize = spec_.plotSize;
    const glm::uvec2 needed = size + glm::uvec2(padding);
    for (Shelf& shelf : plot->shelves) {
        if (needed.y <= shelf.height && shelf.x + needed.x <= plotSize) {
            const glm::uvec2 at = plot->origin + glm::uvec2(shelf.x, shelf.y);
            shelf.x += needed.x;
            return at;
        }
    }
    const std::uint32_t top = plot->shelves.empty() ? padding : plot->shelves.back().y + plot->shelves.back().height;
    if (top + needed.y > plotSize || padding + needed.x > plotSize) {
        return std::nullopt;
    }
    plot->shelves.push_back(Shelf{ .y = top, .height = needed.y, .x = padding + needed.x });
    return plot->origin + glm::uvec2(padding, top);
}

std::optional<std::pair<std::size_t, glm::uvec2>> GlyphStore::room(const glm::uvec2& size) {
    const std::uint32_t padded = 2 * spec_.padding;
    if (size.x + padded > spec_.plotSize || size.y + padded > spec_.plotSize) {
        return std::nullopt;
    }
    // a plot with room, then an empty one
    for (std::size_t plot = 0; plot < plots_.size(); plot++) {
        if (!plots_[plot].shelves.empty()) {
            const std::optional<glm::uvec2> at = place(&plots_[plot], size);
            if (at) {
                return std::make_pair(plot, *at);
            }
        }
    }
    for (std::size_t plot = 0; plot < plots_.size(); plot++) {
        if (plots_[plot].shelves.empty()) {
            return std::make_pair(plot, *place(&plots_[plot], size));
        }
    }
    // then the plot used longest ago, unless the frame being built uses it
    const auto oldest = std::min_element(plots_.begin(), plots_.end(), [](const Plot& one, const Plot& other) { return one.used < other.used; });
    if (pages_.size() < std::max<std::size_t>(spec_.pages, 1) || oldest == plots_.end() || oldest->used >= frame_) {
        addPage();
        const std::size_t plot = plots_.size() - (static_cast<std::size_t>(spec_.pageSize / spec_.plotSize) * (spec_.pageSize / spec_.plotSize));
        return std::make_pair(plot, *place(&plots_[plot], size));
    }
    const auto plot = static_cast<std::size_t>(oldest - plots_.begin());
    empty(plot);
    return std::make_pair(plot, *place(&plots_[plot], size));
}

void GlyphStore::addPage() {
    const std::size_t page = pages_.size();
    pages_.emplace_back(static_cast<std::size_t>(spec_.pageSize) * spec_.pageSize * spec_.channels, 0);
    const std::uint32_t across = spec_.pageSize / spec_.plotSize;
    for (std::uint32_t y = 0; y < across; y++) {
        for (std::uint32_t x = 0; x < across; x++) {
            plots_.push_back(Plot{ .page = page, .origin = glm::uvec2(x, y) * spec_.plotSize, .dirty = true });
        }
    }
}

void GlyphStore::empty(std::size_t plot) {
    Plot& emptied = plots_[plot];
    for (const GlyphKey& key : emptied.keys) {
        entries_.erase(key);
    }
    emptied.keys.clear();
    emptied.shelves.clear();
    std::vector<unsigned char>& pixels = pages_[emptied.page];
    for (std::uint32_t row = 0; row < spec_.plotSize; row++) {
        const std::size_t start = ((static_cast<std::size_t>(emptied.origin.y + row) * spec_.pageSize) + emptied.origin.x) * spec_.channels;
        std::fill_n(pixels.begin() + static_cast<std::ptrdiff_t>(start), static_cast<std::size_t>(spec_.plotSize) * spec_.channels, 0);
    }
    emptied.dirty = true;
}

void GlyphStore::blit(const GlyphBitmap& bitmap, std::size_t page, const glm::uvec2& position) {
    std::vector<unsigned char>& pixels = pages_[page];
    for (std::uint32_t row = 0; row < bitmap.size.y; row++) {
        const std::size_t width = static_cast<std::size_t>(bitmap.size.x) * spec_.channels;
        const auto from = bitmap.pixels.begin() + static_cast<std::ptrdiff_t>(row * width);
        const std::size_t to = ((static_cast<std::size_t>(position.y + row) * spec_.pageSize) + position.x) * spec_.channels;
        std::copy_n(from, width, pixels.begin() + static_cast<std::ptrdiff_t>(to));
    }
}

};  // namespace v3d::font
