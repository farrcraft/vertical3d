/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cmath>

namespace v3d::editor {

/**
 * A document's time: how many frames play in a second, the range of frames that plays, and the
 * frame the scene is posed at. Keys sit on frames, so a change of rate leaves every key on its
 * frame and changes how fast they play.
 **/
struct Timeline final {
    /** Film's rate, which a new document plays at. **/
    static constexpr float DEFAULT_RATE = 24.0F;
    /** Five seconds at that rate. **/
    static constexpr int DEFAULT_END = 120;

    float rate = DEFAULT_RATE;
    int start = 1;
    int end = DEFAULT_END;
    int frame = 1;

    /**
     * @return whether the rate is a positive number, the range runs forwards, and the frame is
     *         within it
     **/
    bool valid() const {
        return std::isfinite(rate) && rate > 0.0F && start <= end && frame >= start && frame <= end;
    }

    bool operator==(const Timeline& other) const = default;
};

};  // namespace v3d::editor
