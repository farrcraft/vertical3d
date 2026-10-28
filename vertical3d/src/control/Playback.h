/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <vertical3d/src/document/Document.h>

#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * Plays the timeline, and steps through it a frame or a key at a time.
 *
 * Playing advances the current frame at the document's frame rate and starts again from the
 * first frame of the range after the last. A key is stepped to as the nearest whole frame
 * within the range.
 **/
class Playback final {
 public:
    explicit Playback(const boost::shared_ptr<Document>& document);

    void play();
    void stop();
    bool playing() const noexcept;

    /**
     * Advance a playing timeline by a step of simulated time.
     * @param step seconds
     **/
    void simulate(float step);

    /**
     * Move the current frame by a number of frames, which may be negative.
     **/
    void step(int frames);

    /**
     * Go to the first or the last frame of the range.
     **/
    void first();
    void last();

    /**
     * Go to the nearest key after the current frame, or before it, or to the first or last
     * key. The keys are those of the items given, or of every item when none is.
     **/
    void nextKey(const Items& items, bool forwards);
    void firstKey(const Items& items);
    void lastKey(const Items& items);

 private:
    /**
     * The frames the items have keys on, or every item's when there are none.
     **/
    std::vector<float> keyed(const Items& items) const;

    boost::shared_ptr<Document> document_;
    bool playing_;
    // seconds played since the current frame began
    float played_;
};

};  // namespace v3d::editor
