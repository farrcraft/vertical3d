/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <optional>
#include <string>

namespace v3d::editor::control {

/**
 * A short message about what a command did, shown at the foot of the active area for a while.
 *
 * A notice starts when it is first drawn rather than when it is posted, so one posted between
 * frames shows for its whole time. It fades in, stays, and fades out, and a new notice replaces
 * the one showing. Times are seconds on the ui's clock.
 **/
class Notices final {
 public:
    /** How long a notice shows, its fades included. **/
    static constexpr float SHOWN = 2.5F;
    /** How long a notice takes to fade in, and to fade out. **/
    static constexpr float FADE = 0.15F;

    /**
     * What to draw: the notice's text, and how opaque it is.
     **/
    struct Shown final {
        std::string text;
        float alpha = 1.0F;
        bool done = true;       /**< marked with a check when it was done, and a cross when refused **/
        bool undoable = false;  /**< whether it says how to undo the change **/
    };

    /**
     * A notice to post: what an operation said, whether it was done, and whether undo takes it
     * back.
     **/
    struct Note final {
        std::string text;
        bool done = true;
        bool undoable = false;
    };

    /**
     * Show a notice in place of any showing. Its first letter is made a capital, since an
     * operation's message is written to start a log line. One with no text is ignored.
     **/
    void post(const Note& note);

    /**
     * @param now the ui clock's time
     * @return the notice to draw now, or nothing when none is showing
     **/
    std::optional<Shown> shown(float now);

 private:
    std::string text_;
    bool done_ = true;
    bool undoable_ = false;
    // when the notice was first drawn, or nothing before then
    std::optional<float> start_;
};

};  // namespace v3d::editor::control
