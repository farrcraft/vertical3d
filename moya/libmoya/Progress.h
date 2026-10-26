/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <functional>

namespace v3d::moya {

/**
 * What a render tells of how far it has got, and whether it was asked to stop.
 **/
class Progress final {
 public:
    /**
     * Told how much of the frame the hider has finished, from 0 to 1, after each bucket of the
     * reyes hider and each row of the ray-trace hider. It returns false to stop the render
     * there. It runs on whatever thread renders.
     **/
    using Callback = std::function<bool(float done)>;

    /**
     * Set what is told the render's progress. An empty callback is told nothing, and the
     * render runs to its end. The callback outlives every world, as an option does not.
     **/
    void callback(const Callback & told);
    /** A render begins, and has not been stopped. **/
    void begin();
    /**
     * Tell the callback how far the hider has got.
     *
     * @return false once the callback has asked to stop, and from then until the next
     *         render begins
     **/
    bool report(float done);
    /** Whether the last render was stopped before its end. **/
    bool stopped() const noexcept;

 private:
    Callback told_;
    bool stopped_ = false;
};

};  // namespace v3d::moya
