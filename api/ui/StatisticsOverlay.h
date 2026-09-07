/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "TextRenderer.h"

#include "../render/realtime/Canvas.h"

#include <boost/shared_ptr.hpp>

namespace v3d::ui {

/**
 * What the loop measured about its own pacing, drawn in the corner of the frame.
 *
 * The lines are text like any other, so this joins the canvas the app is already filling
 * and costs the frame no pass of its own. It starts hidden: the numbers are for whoever is
 * asking why a frame is slow, and are noise the rest of the time.
 **/
class StatisticsOverlay final {
 public:
    /**
     * One frame's measurements, in the units engine::Statistics reports them in.
     *
     * The numbers are copied rather than read from a Statistics because api/ui sits below
     * api/engine and nothing here can name that class. Which is also why the loop hands
     * this over instead of the overlay reaching for it.
     **/
    struct Sample final {
        std::uint64_t mean { 0 };  /**< the mean frame over the window, in nanoseconds **/
        std::uint64_t last { 0 };  /**< the last frame, in nanoseconds **/
        unsigned int steps { 0 };  /**< simulation steps the last frame owed **/
    };

    /**
     * How many lines are drawn, which is what lines() fills.
     **/
    static constexpr std::size_t rows = 3;

    /**
     * @param text the renderer the lines are drawn with, which decides their size
     **/
    /**
     * @param text the font the readout is drawn with, whose atlas is at a base size rather
     *        than at the size this draws - ADR-0036
     * @param size the size to draw the readout at. A frame time is a thing to glance at
     *        rather than read, so it defaults smaller than a ui's own text
     **/
    explicit StatisticsOverlay(const boost::shared_ptr<TextRenderer>& text, float size = defaultSize);

    /**
     * The size the readout is drawn at when the caller names none.
     **/
    static const float defaultSize;

    /**
     * Show the overlay if it is hidden, hide it if it is shown.
     **/
    void toggle() noexcept;

    /**
     * @return whether draw() will put anything on the canvas
     **/
    bool visible() const noexcept;

    /**
     * @param visible whether to draw
     **/
    void visible(bool visible) noexcept;

    /**
     * Append the overlay to a canvas, or nothing at all while it is hidden.
     **/
    void draw(v3d::render::realtime::Canvas* canvas, const Sample& sample);

    /**
     * The lines that are drawn, top to bottom.
     *
     * Separate from draw() so that what a sample reads as can be tested without a font,
     * a device or a window.
     **/
    static std::array<std::string, rows> lines(const Sample& sample);

 private:
    boost::shared_ptr<TextRenderer> text_;
    float size_;
    bool visible_;
};

};  // namespace v3d::ui
