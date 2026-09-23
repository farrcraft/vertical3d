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
 * The part is what lets one upload serve many images - a sprite sheet answers each of its
 * regions with the same texture and a different pair of corners. What a source name means is
 * the app's business, so nothing here reads a sheet. ADR-0020.
 *
 * Not style::property::Image, which is a theme's reference to an image by name and holds one
 * of these once it has been resolved.
 **/
struct Image final {
    /**
     * Nothing, which is what an image is before it has been resolved.
     **/
    Image() noexcept;
    /**
     * The whole of a texture. Implicit, so a resolver with no sheets may answer a bare
     * handle.
     *
     * @param tex the texture
     **/
    Image(const v3d::render::realtime::TextureHandle& tex) noexcept;  // NOLINT(runtime/explicit) - a handle is the whole texture
    /**
     * @param tex the texture
     * @param min the texture coordinate at the image's top left corner
     * @param max the texture coordinate at its bottom right corner
     **/
    Image(const v3d::render::realtime::TextureHandle& tex, const glm::vec2& min, const glm::vec2& max) noexcept;

    /**
     * @return whether there is anything to draw
     **/
    bool valid() const noexcept;

    v3d::render::realtime::TextureHandle texture;
    glm::vec2 uv0;
    glm::vec2 uv1;
};

};  // namespace v3d::ui
