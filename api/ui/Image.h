/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/Handle.h>

#include <glm/glm.hpp>

namespace v3d::ui {

/**
 * What a named image resolves to: a texture, and the part of it that is the image.
 *
 * The part lets one upload serve many images: a sprite sheet resolves each of its regions to
 * the same texture and a different pair of corners. What a source name means is for the app
 * to decide, so nothing here reads a sheet.
 *
 * Not style::property::Image, which is a theme's reference to an image by name and holds one
 * of these once it has been resolved.
 **/
struct Image final {
    /**
     * Nothing: an image before it has been resolved.
     **/
    Image() noexcept;
    /**
     * The whole of a texture. Implicit, so a resolver with no sprite sheets can return a
     * bare handle.
     *
     * @param tex the texture
     **/
    Image(const v3d::render::realtime::TextureHandle& tex) noexcept;  // NOLINT(runtime/explicit) - a handle is the whole texture
    /**
     * The texture coordinates at the image's top left corner and at its bottom right corner.
     **/
    struct Corners {
        glm::vec2 min{0.0F};
        glm::vec2 max{1.0F};
    };

    /**
     * @param tex the texture
     * @param corners the part of it the image is
     **/
    Image(const v3d::render::realtime::TextureHandle& tex, const Corners& corners) noexcept;

    /**
     * @return whether there is anything to draw
     **/
    bool valid() const noexcept;

    v3d::render::realtime::TextureHandle texture;
    glm::vec2 uv0;
    glm::vec2 uv1;
};

};  // namespace v3d::ui
