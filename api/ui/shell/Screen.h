/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/event/Event.h>
#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Interface.h>
#include <api/ui/Immediate.h>
#include <api/ui/Transition.h>
#include <api/ui/paint/ComponentRenderer.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/TextRenderer.h>
#include <api/ui/shell/StatisticsOverlay.h>
#include <api/ui/shell/Tooltip.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
class WindowRenderer;
};  // namespace v3d::render::realtime

namespace v3d::ui::shell {

/**
 * The text an app draws with, the renderers over it, and the canvas they fill, built once
 * over the renderer the app already has.
 *
 * Builds the text renderer, the component renderer and, when Options asks, the statistics
 * overlay and an immediate layer over the WindowRenderer it is handed. It owns the canvas they
 * draw into. An app opens each frame with begin(), which also skips a minimised window.
 *
 * This lives in api/ui because v3dlib_ui links v3dlib_render, so building it on the render
 * side would be a library cycle. WindowRenderer is forward-declared so that no api/ui header names
 * a Vulkan type.
 *
 * The app decides what is drawn into the canvas and which pass it is submitted to. begin()
 * begins the frame and sizes the canvas, and the app fills the canvas and submits it.
 *
 * scale() rebuilds everything that captures the font's size, so draw with the component
 * renderer this returns, and do not keep a reference to it. The immediate layer is kept
 * across a rescale and given the new text callbacks, so the state it holds between frames
 * survives.
 **/
class Screen final {
 public:
    /**
     * Fill in a dressing for text of a size, in pixels. Given the size at the current scale,
     * and called whenever the component renderer is built.
     **/
    typedef std::function<void(paint::Dressing* dressing, float size)> Dress;

    /**
     * What the ui's size follows, besides the scale an app or a user sets.
     **/
    enum class Scaling : std::uint8_t {
        Display,  /**< the monitor's scale, so a tool's text is the same physical size on any monitor **/
        Height    /**< the window's height against Options::reference, so a game's menu fills the same share of its window **/
    };

    /**
     * The window height a game's screen is drawn at a scale of one: a 1080p window.
     **/
    static constexpr float referenceHeight = 1080.0F;

    struct Options final {
        float size { paint::TextRenderer::baseSize };  /**< the font's size at a scale of one **/
        Dress dress;              /**< fills the dressing. When empty, sets only the line height, to 1.4 sizes **/
        bool statistics { true }; /**< build a statistics overlay **/
        bool immediate { false }; /**< build an immediate layer over the same text **/
        Scaling scaling { Scaling::Display };  /**< what the ui's size follows **/
        float reference { referenceHeight };  /**< the window height Scaling::Height draws at a scale of one **/
        paint::ComponentRenderer::Shortcut shortcut;  /**< the shortcut a command shows in a menu and a tooltip. Empty shows none **/
    };

    /**
     * @param engine the renderer the atlas is uploaded through and frames are begun on. Not
     *        owned, and it must outlive this. Null builds everything with no device, which
     *        uploads nothing and begins no frame
     **/
    Screen(v3d::render::realtime::WindowRenderer* engine, const boost::shared_ptr<v3d::asset::Manager>& assets,
        const boost::shared_ptr<v3d::log::Logger>& logger, const Options& options);

    // the text renderer writes into the canvas by its address
    Screen(const Screen&) = delete;
    Screen& operator=(const Screen&) = delete;
    Screen(Screen&&) = delete;
    Screen& operator=(Screen&&) = delete;
    ~Screen() = default;

    /**
     * Begin a frame: draw with the ui's active theme, size the canvas to the frame, clear the
     * canvas, and move the ui's clock on by the real time since the last frame began.
     *
     * The theme is read from the ui every frame, and given to the component renderer and the
     * immediate layer when it has changed, before either draws. Setting the active theme on
     * the Interface is all an app does to change it.
     *
     * @param ui the ui whose theme is drawn with, or null to keep the theme drawn with now
     * @return false when there is no frame to draw into, because the window is minimised or
     *         there is no renderer. The app then draws nothing and returns. For a minimised
     *         window, the empty frame has already been presented
     **/
    bool begin(const Interface* ui);

    /**
     * @return whether the last begin() found the frame a different size from the one before,
     *         which the first one always does. An app resizes its own resources when this is
     *         true
     **/
    bool resized() const noexcept;

    /**
     * Draw larger or smaller, as a user's preference does, rebuilding what captures the size.
     * It multiplies what Options::scaling follows.
     *
     * @param factor how much larger, from a little above zero
     **/
    void scale(float factor);
    float scale() const noexcept;

    /**
     * The monitor's scale changed, or the window moved to a monitor with another. A screen whose
     * Options::scaling is Display is rebuilt at it.
     **/
    void display(float monitor);

    /**
     * @return what every size is multiplied by: scale() times the monitor's scale or the
     *         window's height against Options::reference
     **/
    float factor() const noexcept;

    /**
     * @return the ui's clock, which begin() moves on by the real time between frames, for
     *         whatever an app animates over the ui. It starts at zero
     **/
    const Clock& clock() const noexcept;

    /**
     * @return the shortcut a command shows in a menu, as Options::shortcut gives it, or empty
     *         when it shows none or there is no lookup
     **/
    std::string shortcut(const v3d::event::Event& command) const;

    /**
     * @return the size text is drawn at, which is Options::size times factor()
     **/
    float size() const noexcept;

    /**
     * Draw a ui, then the statistics over it, then the tooltip of what the cursor rests on
     * over both, into the canvas. Either may be missing.
     *
     * A component shows its tooltip once the cursor has rested on it for the dressing's
     * tooltip delay, or for the quick delay when a tooltip was showing a moment before, so
     * moving along a row of buttons shows each one's at once. A press puts it away.
     *
     * @param ui what to draw, or null for nothing but the statistics
     **/
    void draw(const Interface* ui, const StatisticsOverlay::Sample& statistics);

    v3d::render::realtime::canvas::Canvas& canvas() noexcept;
    const boost::shared_ptr<paint::TextRenderer>& text() const noexcept;
    paint::ComponentRenderer& components() noexcept;

    /**
     * @return the overlay, or null when Options::statistics was false
     **/
    const boost::shared_ptr<StatisticsOverlay>& statistics() const noexcept;

    /**
     * @return the immediate layer, or null when Options::immediate was false
     **/
    Immediate* immediate() noexcept;

 private:
    /**
     * The height of a line of text, as a multiple of the text's size.
     **/
    static constexpr float LINE_SPACING = 1.4F;
    /**
     * The Dress used when an app passes none. It sets only the line height, which covers a line
     * of text and the gap after it.
     **/
    static void lineHeight(paint::Dressing* dressing, float size);

    /**
     * Draw the tooltip of what the cursor rests on, once it has rested long enough.
     **/
    void tip(const Interface& ui);

    /**
     * Build what closes over the size, at factor().
     **/
    void build();

    /**
     * Dress the component renderer and the immediate layer from a theme.
     **/
    void theme(const boost::shared_ptr<style::Theme>& theme);

    v3d::render::realtime::WindowRenderer* engine_;
    Options options_;
    float scale_;
    float display_;
    // the factor() the text and the dressings were last built at
    float built_;
    bool resized_;
    // the ui's clock, which transitions are measured on, and when it was last moved on
    Clock clock_;
    std::chrono::steady_clock::time_point last_;
    Tooltip tooltip_;
    v3d::render::realtime::canvas::Canvas canvas_;
    boost::shared_ptr<paint::TextRenderer> text_;
    boost::shared_ptr<paint::ComponentRenderer> components_;
    boost::shared_ptr<StatisticsOverlay> statistics_;
    boost::shared_ptr<Immediate> immediate_;
};

};  // namespace v3d::ui::shell
