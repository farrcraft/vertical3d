/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/Handle.h>

#include <cstddef>
#include <vector>

#include "World.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::canvas {

/**
 * World quads collected with a depth key and handed to a canvas furthest first.
 *
 * The caller supplies the key. In an isometric projection it is how far up the ground plane a
 * sprite's feet are, which nothing here can compute. A larger key is further away and is drawn
 * first. Equal keys are drawn grouped by texture and otherwise in the order they were added. A
 * caller that needs fewer batch cuts quantises its key - to a tile row, say - and lets the
 * tie-break merge them. A key that is not a number is drawn last.
 *
 * World itself still draws in submission order. This is one way of choosing that order.
 **/
class DepthOrder final {
 public:
    /**
     * Drop everything collected, keeping the capacity.
     **/
    void clear();

    /**
     * An untextured quad, drawn against the renderer's white texture.
     * @param key how far away the quad is, in whatever measure the caller sorts by
     **/
    void quad(float key, const World::Corners& corners, const glm::vec4& colour);

    /**
     * A quad sampling a region of a texture, with World::quad's meaning for the rest.
     * @param key how far away the quad is, in whatever measure the caller sorts by
     **/
    void quad(float key, const World::Corners& corners, const glm::vec2& uv0,
        const glm::vec2& uv1, const glm::vec4& colour, const TextureHandle& texture);

    /**
     * Add everything collected to a canvas, furthest first. The canvas's current transform
     * applies to it as it would to quads added directly.
     **/
    void into(World* canvas) const;

    /**
     * @return how many quads have been collected
     **/
    std::size_t size() const noexcept;

 private:
    struct Entry {
        float key;
        World::Corners corners;
        glm::vec2 uv0;
        glm::vec2 uv1;
        glm::vec4 colour;
        TextureHandle texture;  /**< unset for an untextured quad **/
    };

    std::vector<Entry> entries_;
};

};  // namespace v3d::render::realtime::canvas
