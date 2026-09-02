/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <deque>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime {

    /**
     * Everything the engine draws as lines, accumulated as one stream of segments.
     *
     * The cpu half of the line primitive of ADR-0011, and the counterpart of Canvas: filled
     * during a tick and handed to vulkan::LineRenderer, which draws the whole stream as one
     * line list. There is no index buffer - segments rarely share a vertex, so indexing a
     * line list costs more than it saves.
     *
     * Nothing here touches vulkan.
     *
     * Coordinates are in world space, not in pixels. A line canvas draws through the camera
     * its pass carries at set 0, so a pass whose camera was never set draws it in clip
     * space.
     **/
    class LineCanvas final {
     public:
        /**
         * One vertex of the stream, in the layout the line pipeline declares.
         **/
        struct Vertex final {
            glm::vec3 position;
            glm::vec4 colour;
        };

        /**
         **/
        LineCanvas();

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
        void transform(const glm::mat4& transform);  // NOLINT(build/include_what_you_use) - the name, not std::transform

        /**
         * Translate the current transform.
         **/
        void translate(const glm::vec3& offset);

        /**
         * @return the transform vertices are being written through
         **/
        const glm::mat4& transform() const noexcept;

        /**
         * One segment.
         **/
        void line(const glm::vec3& from, const glm::vec3& to, const glm::vec4& colour);

        /**
         * A run of connected segments.
         * @param points at least two, or nothing is added
         * @param closed whether to join the last point back to the first
         **/
        void polyline(const std::vector<glm::vec3>& points, const glm::vec4& colour, bool closed = false);

        /**
         * The twelve edges of an axis aligned box.
         **/
        void box(const glm::vec3& min, const glm::vec3& max, const glm::vec4& colour);

        /**
         * A closed ring, approximated by segments.
         *
         * The plane is given as two axes rather than a normal, which would leave the ring
         * free to spin about it. The axes need be neither unit length nor perpendicular, so
         * an ellipse is expressible.
         *
         * @param axisU where the ring starts, scaled by the radius
         * @param axisV a quarter turn on from it, scaled by the radius
         * @param sides how many segments to approximate it with - fewer than three draws
         *        nothing
         **/
        void circle(const glm::vec3& centre, const glm::vec3& axisU, const glm::vec3& axisV, float radius,
            unsigned int sides, const glm::vec4& colour);

        /**
         * @return the vertex stream, two vertices per segment, in the layout the line
         *         pipeline declares
         **/
        const std::vector<Vertex>& vertices() const noexcept;

        /**
         * @return whether there is anything to draw
         **/
        bool empty() const noexcept;

     private:
        /**
         * Add a vertex with the current transform applied.
         **/
        void vertex(const glm::vec3& position, const glm::vec4& colour);

        std::deque<glm::mat4> transforms_;
        std::vector<Vertex> vertices_;
    };

};  // namespace v3d::render::realtime
