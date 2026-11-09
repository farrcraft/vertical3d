/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/ui/Clock.h>

#include <cstdint>

namespace v3d::ui {

/**
 * How far something has come since it began to be drawn: 0 on the frame it appears, rising to 1
 * over a duration.
 *
 * Something that was not drawn on the frame before begins again, so a menu eases in each time it
 * opens. Closing needs nothing, because what is closed is not drawn.
 **/
class Opening final {
 public:
    /**
     * @param clock the time and frame this is drawn at
     * @param duration how long opening takes, in seconds. Zero or less is open at once
     * @return how far open, from 0 to 1
     **/
    float progress(const Clock& clock, float duration);

 private:
    float start_ = 0.0F;
    std::uint64_t frame_ = 0;
    bool seen_ = false;
};

};  // namespace v3d::ui
