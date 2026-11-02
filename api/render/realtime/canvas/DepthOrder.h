/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/Handle.h>

#include <cstddef>
#include <optional>
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
 * first. At equal keys, effects drawn from distance fields come first, by their layer, so that
 * they lie beneath the glyphs they belong to. Otherwise equal keys are drawn grouped by
 * texture, and glyphs by their bands, and otherwise in the order they were added. A caller
 * that needs fewer batch cuts quantises its key, to a tile row for example, and lets
 * the tie-break merge them. A key that is not a number is drawn last.
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
     * A glyph drawn from its curves, with World::glyph's meaning for the rest.
     * @param key how far away the glyph is, in whatever measure the caller sorts by
     **/
    void glyph(float key, const World::Corners& corners, const World::Glyph& glyph, const glm::vec4& colour);

    /**
     * An effect drawn from a distance field, with World::field's meaning for the rest.
     * @param key how far away the effect is, in whatever measure the caller sorts by
     * @param layer the effect's place among a glyph's effects. At equal keys a lower layer is
     *        drawn first, and every field before anything else
     **/
    void field(float key, uint32_t layer, const World::Corners& corners, const glm::vec2& uv0, const glm::vec2& uv1,
        const glm::vec4& colour, const TextureHandle& texture, uint32_t ramp);

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
        std::optional<World::Glyph> glyph;  /**< set for a glyph drawn from its curves **/
        std::optional<uint32_t> ramp;       /**< set for an effect drawn from a distance field **/
        uint32_t layer = 0;                 /**< an effect's place among its glyph's effects **/
    };

    std::vector<Entry> entries_;
};

};  // namespace v3d::render::realtime::canvas
