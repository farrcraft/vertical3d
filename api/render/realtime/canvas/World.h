/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/resource/Handle.h>

#include <array>
#include <cstdint>
#include <vector>

#include "Stacks.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::canvas {

/**
 * Everything the engine draws as textured quads standing in the world, accumulated as one
 * stream.
 *
 * The CPU side of the world space quad primitive. It draws the same quads as Canvas, but in
 * world space as Line does. A quad here has four world corners and is drawn through
 * the camera its pass carries at set 0, so a pass whose camera was never set draws it in
 * clip space.
 *
 * Nothing here calls Vulkan.
 *
 * **The caller sets the draw order.** Quads are drawn in the order they were added, because
 * a quad's depth means something only to the caller. In an isometric projection a sprite is
 * behind another when its feet are further up the ground plane, not when it is further from
 * the camera. Depth testing hides a quad behind solid geometry and never behind another
 * quad.
 *
 * Batching follows Canvas: the stream cuts where the bound texture changes, and an
 * untextured quad names no texture and is drawn against the renderer's 1x1 white one.
 **/
class World final {
 public:
    /**
     * One vertex of the stream, in the layout the world quad pipeline declares.
     **/
    struct Vertex final {
        glm::vec3 position;
        glm::vec2 uv;
        glm::vec4 colour;
        uint32_t glyph = 0;  /**< where a curve glyph's data starts among its bands, or a distance field's ramp. Every other quad leaves it 0 **/
        float contrast = -1.0F;  /**< for text, how much its coverage is strengthened, from 0 to 1. Below 0 for what is not text **/
    };

    /**
     * A glyph drawn from its curves, as Canvas::curves() draws one: the place in the em square
     * each corner of its quad carries, with y upwards, and where its data starts among the bands.
     **/
    struct Glyph final {
        std::array<glm::vec2, 4> em{};
        uint32_t bands = 0;
        resource::TextureHandle curveTexture;  /**< the float texture of the glyph's curves **/
        resource::TextureHandle bandTexture;   /**< and of its bands **/
        float contrast = 0.0F;       /**< how much its coverage is strengthened, from 0 to 1 **/
    };

    /**
     * A run of indices that can be drawn with one call, because everything in it samples
     * the same texture.
     **/
    struct Batch final {
        Batch() noexcept;

        resource::TextureHandle texture;  /**< unset for the untextured quads drawn against white **/
        resource::TextureHandle bands;    /**< set for a run of curve glyphs, whose curves are in texture **/
        bool field;             /**< whether the run samples a distance field **/
        uint32_t firstIndex;    /**< where the run starts in indices() **/
        uint32_t indices;       /**< how long the run is **/
    };

    /**
     * The four corners of a quad, in order around its perimeter so that consecutive corners
     * share an edge. grid::tileCorners returns them in this order.
     **/
    typedef std::array<glm::vec3, 4> Corners;

    /**
     **/
    World();

    /**
     * Drop everything accumulated and reset the transform stack and the tint, keeping the
     * capacity. Called at the start of a tick, since the stream is rebuilt every frame.
     **/
    void clear();

    /**
     * Push a copy of the current transform onto the modelview stack.
     **/
    void push();

    /**
     * Pop the modelview stack, back to the transform push() saved.
     **/
    void pop();

    /**
     * Apply a transform on top of the current one, so geometry can be described at the
     * origin and placed where it is drawn.
     **/
    void transform(const glm::mat4& applied);  // NOLINT(build/include_what_you_use) - the name, not std::transform

    /**
     * Translate the current transform.
     **/
    void translate(const glm::vec3& offset);

    /**
     * @return the transform vertices are being written through
     **/
    const glm::mat4& transform() const noexcept;

    /**
     * Multiply every quad added from here on by a colour, until the tint is set again. It is
     * a light over the whole world, such as dusk or an act's palette, set once rather than by
     * every caller. White leaves the colours alone.
     *
     * It is baked into the quads, so it suits a canvas built every frame, or part of one. A
     * canvas kept across frames is lit by the tint renderer::World::submit() takes instead,
     * which changes without building the canvas again.
     **/
    void tint(const glm::vec4& colour);

    /**
     * @return the colour quads are being multiplied by
     **/
    const glm::vec4& tint() const noexcept;

    /**
     * An untextured quad, drawn against the renderer's white texture.
     **/
    void quad(const Corners& corners, const glm::vec4& colour);

    /**
     * A quad sampling a region of a texture.
     *
     * The uv rectangle is laid over the corners in the order they are given, so the first
     * corner takes uv0 and the third takes uv1 and the other two take one of each.
     *
     * @param uv0 the texture coordinate at the first corner
     * @param uv1 the texture coordinate at the third
     * @param colour multiplied with what is sampled, so white leaves the texture alone
     **/
    void quad(const Corners& corners, const glm::vec2& uv0, const glm::vec2& uv1,
        const glm::vec4& colour, const resource::TextureHandle& texture);

    /**
     * A glyph drawn from its curves over a quad, sharp at any distance and angle. Its corners
     * take the glyph's em corners in the order they are given. A run of glyphs never merges with
     * quads drawn another way.
     **/
    void glyph(const Corners& corners, const Glyph& glyph, const glm::vec4& colour);

    /**
     * An effect drawn from a glyph's distance field over a quad, such as an outline, a shadow
     * or a glow, with quad()'s meaning for the corners and the uv rectangle. A run of fields
     * never merges with quads drawn another way.
     *
     * @param ramp where the effect is whole and where it is gone, from fieldRamp()
     * @param contrast how much its coverage is strengthened, as text's is
     **/
    void field(const Corners& corners, const glm::vec2& uv0, const glm::vec2& uv1, const glm::vec4& colour,
        const resource::TextureHandle& texture, uint32_t ramp, float contrast = 0.0F);

    /**
     * @return the vertex stream, in the layout the world quad pipeline declares
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
    // two triangles
    static constexpr uint32_t QUAD_INDICES = 6;

    /**
     * Start a batch, or extend the open one when it draws against the same texture and bands,
     * and is a field or not as it is.
     **/
    void open(const resource::TextureHandle& texture, const resource::TextureHandle& bands = resource::TextureHandle(), bool field = false);

    /**
     * Add a vertex with the current transform and tint applied.
     **/
    void vertex(const Vertex& untransformed);

    /**
     * Index the four vertices starting at first as two triangles, fanned from the first
     * corner so that any convex quad given in perimeter order comes out whole.
     **/
    void fan(uint32_t first);

    TransformStack transforms_;
    glm::vec4 tint_;
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<Batch> batches_;
};

};  // namespace v3d::render::realtime::canvas
