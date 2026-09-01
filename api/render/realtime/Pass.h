/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "DrawItem.h"

#include <glm/vec4.hpp>

namespace v3d::render::realtime {

    /**
     * One pass of a frame - a target, what to do with what is already in it, a camera, and
     * the draw items to record into it.
     *
     * The pass is the unit of variation between 2D and 3D drawing rather than the engine
     * being, per ADR-0003: a sprite pass is one with no depth buffer and painter ordering,
     * a scene pass is one with depth and front to back ordering, and a viewport of an editor
     * is one more pass over the same device. A frame holds a list of them even while there
     * is only ever one, because retrofitting the list later is the expensive version.
     *
     * Only the swapchain image is a valid target so far, so a pass has no target field yet -
     * offscreen targets arrive with compositing.
     **/
    class Pass final {
     public:
        /**
         * @param name what the pass is for, used in logs and debug markers
         **/
        explicit Pass(const std::string& name);

        /**
         * @return the name the pass was created with
         **/
        const std::string& name() const noexcept;

        /**
         * Clear the target to a colour before anything in the pass draws.
         * A pass that does not clear draws over whatever the pass before it left, which
         * means the first pass of a frame should always clear.
         **/
        void clearColour(const glm::vec4& colour) noexcept;

        /**
         * Leave whatever is in the target and draw over it.
         **/
        void keepColour() noexcept;

        /**
         * @return whether the pass clears its target before drawing
         **/
        bool clears() const noexcept;

        /**
         * @return the colour the target is cleared to
         **/
        const glm::vec4& clearColour() const noexcept;

        /**
         * Whether the pass depth tests. 2D passes do not - they rely on painter ordering.
         **/
        void depth(bool enabled) noexcept;

        /**
         * @return whether the pass depth tests
         **/
        bool depth() const noexcept;

        /**
         * The region of the target the pass draws into, as x, y, width, height in pixels.
         * A width or height of zero means the whole target, which is the default and what
         * every pass wants until an editor draws four viewports of one scene.
         **/
        void viewport(const glm::vec4& region) noexcept;

        /**
         * @return the region of the target the pass draws into
         **/
        const glm::vec4& viewport() const noexcept;

        /**
         * Add a draw item to the pass. The engine decides when it is recorded.
         **/
        void submit(const DrawItem& item);

        /**
         * @return the items submitted to the pass, in submission order
         **/
        const std::vector<DrawItem>& items() const noexcept;

        /**
         * Drop the submitted items, keeping the pass's configuration. Called at the end of
         * a frame, so the next one starts from an empty queue without reallocating.
         **/
        void reset() noexcept;

     private:
        std::string name_;
        glm::vec4 clearColour_;
        glm::vec4 viewport_;
        std::vector<DrawItem> items_;
        bool clears_;
        bool depth_;
    };

};  // namespace v3d::render::realtime
