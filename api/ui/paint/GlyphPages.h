/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/GlyphStore.h>
#include <api/render/realtime/Handle.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::ui::paint {

/**
 * The device's copy of a glyph store's pages, one texture for each, kept in step with the store.
 *
 * sync() is called once a frame, after the frame's glyphs have been asked of the store and
 * before the frame is submitted. A page the store added becomes a texture, a plot it changed is
 * copied into its page's texture, and a page it let go is released. A plot is copied while
 * earlier frames may still draw the page, as a region update allows.
 *
 * A page's pixels have the store's channels: one byte, or four for a store of colour glyphs.
 *
 * The device is reached through callbacks, so the ui names no renderer. With a context's
 * Textures, create is textures->texture(pixels, side, side, channels), update is
 * textures->update() with the channels given, and release is textures->release().
 **/
class GlyphPages final {
 public:
    /**
     * Make a texture from a page: side pixels square, each of a number of channels.
     **/
    using Create = std::function<render::realtime::TextureHandle(const unsigned char* pixels, std::uint32_t side, std::uint32_t channels)>;

    /**
     * Copy rows of pixels of a number of channels into a region of a texture, its top left
     * texel at a place.
     **/
    using Update = std::function<void(const render::realtime::TextureHandle& texture, const glm::uvec2& at, const unsigned char* pixels,
        const glm::uvec2& size, std::uint32_t channels)>;

    /**
     * Release a page's texture.
     **/
    using Release = std::function<void(const render::realtime::TextureHandle& texture)>;

    GlyphPages(Create create, Update update, Release release);

    /**
     * Bring the textures in step with the store.
     **/
    void sync(font::GlyphStore* store);

    /**
     * @return a page's texture, or an unset handle for a page not yet synced
     **/
    render::realtime::TextureHandle texture(std::size_t page) const;

    /**
     * @return how many pages have a texture
     **/
    std::size_t pages() const noexcept;

 private:
    Create create_;
    Update update_;
    Release release_;
    std::vector<render::realtime::TextureHandle> textures_;
    std::vector<unsigned char> rows_;  // a plot's pixels, gathered from its page for a copy
};

};  // namespace v3d::ui::paint
