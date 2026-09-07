/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <deque>
#include <vector>

#include "Handle.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::font {
class TextBuffer;
};  // namespace v3d::font

namespace v3d::render::realtime {

/**
 * Everything 2D the engine draws, accumulated as one stream of textured quads.
 *
 * This is the cpu half of the one batched quad primitive of ADR-0005: a coloured
 * rectangle, a sprite and a glyph are the same quad with a different texture, so they
 * share one vertex buffer and the stream is cut into a new batch only where the texture
 * changes. An untextured quad names no texture and is drawn against the renderer's 1x1
 * white one, so it never cuts a batch of its own.
 *
 * Nothing here touches vulkan. The canvas is filled during a tick and handed to
 * vulkan::QuadRenderer, which uploads it and turns each batch into a draw item.
 *
 * Coordinates are in pixels with the origin at the top left, and the modelview stack
 * applies on the cpu as vertices are added.
 **/
class Canvas final {
 public:
    /**
     * One vertex of the stream, in the layout the quad pipeline declares.
     **/
    struct Vertex final {
        glm::vec2 position;
        glm::vec2 uv;
        glm::vec4 colour;
    };

    /**
     * A run of indices that can be drawn with one call, because everything in it samples
     * the same texture.
     **/
    struct Batch final {
        Batch() noexcept;

        TextureHandle texture;  /**< unset for the untextured quads drawn against white **/
        uint32_t firstIndex;    /**< where the run starts in indices() **/
        uint32_t indices;       /**< how long the run is **/
    };

    /**
     **/
    Canvas();

    /**
     * Drop everything accumulated, keeping the capacity and the canvas size. Called at
     * the start of a tick, since the whole stream is rebuilt every frame.
     **/
    void clear();

    /**
     * The size of the area being drawn into, which is what projection() maps from.
     **/
    void resize(uint32_t width, uint32_t height);

    /**
     * @return the width the canvas was last sized to
     **/
    uint32_t width() const noexcept;

    /**
     * @return the height the canvas was last sized to
     **/
    uint32_t height() const noexcept;

    /**
     * The transform from canvas pixels to clip space, for the pipeline's push constant.
     *
     * Built by hand rather than with glm::ortho, whose y direction and depth range
     * depend on how glm was configured when it was compiled.
     *
     * @return an orthographic projection with the origin at the top left
     **/
    glm::mat4 projection() const;

    /**
     * Push a copy of the current transform onto the modelview stack.
     **/
    void push();

    /**
     * Pop the modelview stack, back to the transform push() saved.
     **/
    void pop();

    /**
     * Translate the current transform.
     **/
    void translate(const glm::vec2& offset);

    /**
     * An untextured rectangle, drawn against the renderer's white texture.
     * @param min the top left corner in pixels
     * @param max the bottom right corner in pixels
     **/
    void rect(const glm::vec2& min, const glm::vec2& max, const glm::vec4& colour);

    /**
     * A rectangle sampling a region of a texture.
     * @param uv0 the texture coordinate at min
     * @param uv1 the texture coordinate at max
     * @param colour multiplied with what is sampled, so white leaves the texture alone
     **/
    void rect(const glm::vec2& min, const glm::vec2& max, const glm::vec2& uv0, const glm::vec2& uv1,
        const glm::vec4& colour, const TextureHandle& texture);

    /**
     * A filled circle, as a fan of triangles.
     * @param sides how many segments to approximate it with
     **/
    void circle(const glm::vec2& centre, float radius, unsigned int sides, const glm::vec4& colour);

    /**
     * A filled wedge of a circle, as a fan of triangles.
     *
     * Angles are radians and turn the way the canvas does, which is clockwise on screen
     * because y grows downwards: zero points right, a quarter turn points down.
     *
     * @param sides how many segments to approximate the sweep with
     * @param start where the wedge begins
     * @param sweep how far round it goes
     **/
    void arc(const glm::vec2& centre, float radius, unsigned int sides, float start, float sweep,
        const glm::vec4& colour);

    /**
     * Append text that a font's text buffer has already laid out.
     *
     * The buffer holds positions, atlas coordinates and colours, so this copies them
     * into the stream against the atlas texture. Laying the text out is the font
     * library's job.
     *
     * @param text a laid out text buffer, whose contents are copied rather than kept
     * @param atlas the texture the glyphs were packed into
     **/
    void text(const v3d::font::TextBuffer& text, const TextureHandle& atlas);

    /**
     * @return the vertex stream, in the layout the quad pipeline declares
     **/
    const std::vector<Vertex>& vertices() const noexcept;

    /**
     * @return the index stream every batch indexes into
     **/
    const std::vector<uint32_t>& indices() const noexcept;

    /**
     * @return the runs the stream is cut into, in the order they are drawn
     **/
    const std::vector<Batch>& batches() const noexcept;

    /**
     * @return whether there is anything to draw
     **/
    bool empty() const noexcept;

 private:
    /**
     * Start a batch, or extend the open one when it already draws with this texture.
     **/
    void open(const TextureHandle& texture);

    /**
     * Add a vertex with the current transform applied.
     **/
    void vertex(const glm::vec2& position, const glm::vec2& uv, const glm::vec4& colour);

    /**
     * Two triangles over the last four vertices added, wound counter clockwise.
     **/
    void quad(uint32_t first);

    uint32_t width_;
    uint32_t height_;
    std::deque<glm::mat4> transforms_;
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<Batch> batches_;
};

};  // namespace v3d::render::realtime
