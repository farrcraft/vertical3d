/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <vector>

#include "Handle.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime {

/**
 * Everything the engine draws as textured quads standing in the world, accumulated as one
 * stream.
 *
 * The cpu half of the third primitive of ADR-0042, and the counterpart of Canvas in space
 * and of LineCanvas in primitive: a quad here has four world corners and is drawn through
 * the camera its pass carries at set 0, so a pass whose camera was never set draws it in
 * clip space.
 *
 * Nothing here touches vulkan.
 *
 * **The order is the caller's.** Quads are drawn in the order they were added, because what
 * a quad's depth means is the caller's knowledge - in an isometric projection a sprite is
 * behind another when its feet are further up the ground plane, not when it is further from
 * the camera. Depth testing hides a quad behind solid geometry and never behind another
 * quad, per ADR-0042.
 *
 * Batching is ADR-0005's rule unchanged: the stream cuts where the bound texture changes,
 * and an untextured quad names no texture and is drawn against the renderer's 1x1 white one.
 **/
class WorldCanvas final {
 public:
    /**
     * One vertex of the stream, in the layout the world quad pipeline declares.
     **/
    struct Vertex final {
        glm::vec3 position;
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
     * The four corners of a quad, in order around its perimeter so that consecutive corners
     * share an edge - the order grid::tileCorners hands them out in.
     **/
    typedef std::array<glm::vec3, 4> Corners;

    /**
     **/
    WorldCanvas();

    /**
     * Drop everything accumulated and reset the transform stack, keeping the capacity.
     * Called at the start of a tick, since the stream is rebuilt every frame.
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
        const glm::vec4& colour, const TextureHandle& texture);

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
    /**
     * Start a batch, or extend the open one when it draws against the same texture.
     **/
    void open(const TextureHandle& texture);

    /**
     * Add a vertex with the current transform applied.
     **/
    void vertex(const glm::vec3& position, const glm::vec2& uv, const glm::vec4& colour);

    /**
     * Index the four vertices starting at first as two triangles, fanned from the first
     * corner so that any convex quad given in perimeter order comes out whole.
     **/
    void fan(uint32_t first);

    std::deque<glm::mat4> transforms_;
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<Batch> batches_;
};

};  // namespace v3d::render::realtime
