/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

class Scene;

/**
 * The F3 overlay - the build, a rolling frame rate and where the player is standing.
 *
 * It produces lines of text and nothing else. Drawing them is the renderer's, because the
 * font and the canvas the ui is already drawn through are what a line of text goes onto.
 **/
class DebugOverlay {
 public:
    explicit DebugOverlay(const boost::shared_ptr<Scene>& scene);

    void enable(bool status);
    bool enabled() const;

    /**
     * Fold a frame's duration into the rolling average and rebuild the lines.
     * @param delta how long the frame took, in milliseconds
     **/
    void update(unsigned int delta);

    /**
     * @return what to draw, one line per entry, as of the last update
     **/
    const std::vector<std::string>& lines() const;

 private:
    /**
     * How many frames the frame rate is averaged over. A single frame's duration jitters
     * enough to be unreadable.
     **/
    static const size_t samples = 100;

    /**
     * @return the average frame duration in milliseconds, which ramps up until the window
     *         has been filled once
     **/
    unsigned int average(unsigned int delta);

    boost::shared_ptr<Scene> scene_;
    bool enabled_;
    std::array<unsigned int, samples> durations_;
    size_t cursor_;
    unsigned int total_;
    std::vector<std::string> lines_;
};
