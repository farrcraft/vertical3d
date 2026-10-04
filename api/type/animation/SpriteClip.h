/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "Clock.h"

#include <glm/vec2.hpp>

namespace v3d::type::animation {

/**
 * A sprite's frames: a sequence of regions of a sheet, each shown for its own length of time.
 *
 * Regions are held resolved, as the uv pair a canvas takes, rather than by name. A game resolves
 * them from its sheet when it loads and builds the clip again when the sheet is reloaded.
 *
 * Like the Clock it runs on, a clip holds no time of its own: a walk cycle is played by the
 * time the game keeps, and a particle's look by the particle's age.
 **/
class SpriteClip final {
 public:
    /**
     **/
    struct Frame final {
        glm::vec2 uv0;   /**< the region's top-left **/
        glm::vec2 uv1;   /**< the region's bottom-right **/
        float duration;  /**< in seconds **/
    };

    /**
     * @param frames in the order they are shown
     * @param loops whether the clip starts again after its last frame, or holds it
     * @throw std::invalid_argument for no frames, or a frame that is never shown
     **/
    SpriteClip(const std::vector<Frame>& frames, bool loops);

    /**
     * @return the clock the frames run on, whose duration is their summed length
     **/
    const Clock& clock() const noexcept;

    /**
     * @param time where playback stands, unwrapped, as the clock advances it
     * @return the frame showing at that time. A frame is shown from its start up to, but not
     *         including, the start of the next; a clamped clip holds its last frame at its end
     **/
    const Frame& frame(float time) const noexcept;

    /**
     **/
    const std::vector<Frame>& frames() const noexcept;

 private:
    std::vector<Frame> frames_;
    std::vector<float> ends_;  /**< where each frame stops showing, rising **/
    Clock clock_;
};

};  // namespace v3d::type::animation
