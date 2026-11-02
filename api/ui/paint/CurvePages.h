/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/CurveStore.h>
#include <api/render/realtime/Handle.h>

#include <cstdint>
#include <functional>

#include <glm/vec2.hpp>

namespace v3d::ui::paint {

/**
 * The device's copy of a curve store's two arrays, a float texture for each, kept in step with
 * the store.
 *
 * sync() is called once a frame, after the frame's glyphs have been asked of the store and
 * before the frame is submitted. An array with more rows than its texture becomes a new texture,
 * and the old one is released. Otherwise the rows the store changed are copied into its texture,
 * while earlier frames may still read it.
 *
 * The device is reached through callbacks, so the ui names no renderer. With a context's
 * Textures, create is textures->texture() of the texels, update is textures->update() of them,
 * and release is textures->release().
 **/
class CurvePages final {
 public:
    /**
     * Make a float texture of four floats a texel, width by height texels.
     **/
    using Create = std::function<render::realtime::TextureHandle(const float* texels, std::uint32_t width, std::uint32_t height)>;

    /**
     * Copy rows of four floats a texel into a region of a float texture, at a place.
     **/
    using Update = std::function<void(const render::realtime::TextureHandle& texture, const glm::uvec2& at, const float* texels,
        const glm::uvec2& size)>;

    /**
     * Release a texture.
     **/
    using Release = std::function<void(const render::realtime::TextureHandle& texture)>;

    CurvePages(Create create, Update update, Release release);

    /**
     * Bring the textures in step with the store.
     **/
    void sync(font::CurveStore* store);

    /**
     * @return the curves' texture, unset until the store holds a curve and has been synced
     **/
    render::realtime::TextureHandle curves() const noexcept;

    /**
     * @return the bands' texture, likewise
     **/
    render::realtime::TextureHandle bands() const noexcept;

 private:
    /**
     * One array's texture and how many rows it was made with.
     **/
    struct Page {
        render::realtime::TextureHandle texture;
        std::uint32_t rows = 0;
    };

    /**
     * Bring one array's texture in step with its rows.
     **/
    void sync(Page* page, const float* floats, std::uint32_t rows, const font::CurveStore::Changed& changed);

    Create create_;
    Update update_;
    Release release_;
    Page curves_;
    Page bands_;
};

};  // namespace v3d::ui::paint
