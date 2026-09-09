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
 * A glyph is the exception ADR-0036 adds: its atlas holds distances rather than coverage,
 * so a batch also records whether it is text and the stream cuts where that changes. A
 * sprite drawn from a glyph atlas and a label sample the same texture and must not merge.
 *
 * Nothing here touches vulkan. The canvas is filled during a tick and handed to
 * vulkan::renderer::Quad, which uploads it and turns each batch into a draw item.
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
     * the same texture the same way and is cut to the same rectangle.
     **/
    struct Batch final {
        Batch() noexcept;

        TextureHandle texture;  /**< unset for the untextured quads drawn against white **/
        bool text;              /**< whether the run samples a distance field - ADR-0036 **/
        bool clipped;           /**< whether clip cuts the run down, per ADR-0037 **/
        glm::vec4 clip;         /**< the rectangle it is cut to - min x, min y, max x, max y **/
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
     * Scale the current transform about the origin it is already translated to.
     **/
    void scale(const glm::vec2& factor);

    /**
     * Cut everything drawn until the matching unclip() to a rectangle.
     *
     * The rectangle is in the coordinates being drawn in - the current transform applies
     * to it the same way it applies to a vertex, and it is resolved once, here, so
     * translating afterwards moves what is drawn rather than what it is cut to. It is
     * intersected with whatever is already clipped, so an inner clip can only take room
     * away.
     *
     * Clipping is per batch and not per vertex: the stream cuts where the rectangle
     * changes and the device scissors the draw, so a quad straddling the edge is drawn
     * whole and half of it lands. ADR-0037.
     **/
    void clip(const glm::vec2& min, const glm::vec2& max);

    /**
     * Go back to what was clipped before the matching clip(). Clipping nothing at the
     * bottom of the stack means the whole canvas.
     **/
    void unclip();

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
     * A filled band between two radii, as a strip of triangles.
     *
     * The arc that traces a rounded corner rather than the wedge that fills one, so an
     * outline around a rounded box costs no second shape and covers nothing inside it.
     * Angles are as arc() takes them. An inner radius of zero or less is a wedge, and
     * this is arc().
     *
     * @param outer the radius the band ends at
     * @param inner the radius it starts at, which is the hole it leaves
     **/
    void ring(const glm::vec2& centre, float outer, float inner, unsigned int sides, float start,
        float sweep, const glm::vec4& colour);

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
     * Start a batch, or extend the open one when it already draws the same way.
     *
     * The texture, the text flag and the clip rectangle all have to match: the fragment
     * shader treats a text batch's texels as distances, so merging one with a run of
     * sprites would threshold the sprites, and a scissor belongs to a whole draw, so two
     * runs cut differently cannot be one.
     **/
    void open(const TextureHandle& texture, bool text = false);

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
    /**< what each open clip cuts to, already transformed and intersected; empty is uncut **/
    std::deque<glm::vec4> clips_;
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<Batch> batches_;
};

};  // namespace v3d::render::realtime
