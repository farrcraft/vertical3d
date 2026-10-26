/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Interface.h>
#include <api/ui/Immediate.h>
#include <api/ui/paint/ComponentRenderer.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/TextRenderer.h>
#include <api/ui/shell/StatisticsOverlay.h>

#include <functional>

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

    struct Options final {
        float size { paint::TextRenderer::baseSize };  /**< the font's size at a scale of one **/
        Dress dress;              /**< fills the dressing. When empty, sets only the line height, to 1.4 sizes **/
        bool statistics { true }; /**< build a statistics overlay **/
        bool immediate { false }; /**< build an immediate layer over the same text **/
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
     * Begin a frame: size the canvas to it, and clear the canvas.
     *
     * @return false when there is no frame to draw into, because the window is minimised or
     *         there is no renderer. The app then draws nothing and returns. For a minimised
     *         window, the empty frame has already been presented
     **/
    bool begin();

    /**
     * @return whether the last begin() found the frame a different size from the one before,
     *         which the first one always does. An app resizes its own resources when this is
     *         true
     **/
    bool resized() const noexcept;

    /**
     * Draw text larger or smaller, rebuilding what captures its size. The dressing is
     * filled in again for the new size, and the theme is set again on what is rebuilt.
     *
     * @param factor how much larger than Options::size, from a little above zero
     **/
    void scale(float factor);
    float scale() const noexcept;

    /**
     * @return the size text is drawn at, which is the size the screen was built for times
     *         the scale
     **/
    float size() const noexcept;

    /**
     * Set a theme on the component renderer and the immediate layer, and keep it for what
     * scale() rebuilds.
     **/
    void theme(const boost::shared_ptr<style::Theme>& theme);

    /**
     * Draw a ui, then the statistics over it, into the canvas. Either may be missing.
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
     * Build what closes over the size, at the current scale.
     **/
    void build();

    v3d::render::realtime::WindowRenderer* engine_;
    Options options_;
    float scale_;
    bool resized_;
    v3d::render::realtime::canvas::Canvas canvas_;
    boost::shared_ptr<paint::TextRenderer> text_;
    boost::shared_ptr<paint::ComponentRenderer> components_;
    boost::shared_ptr<StatisticsOverlay> statistics_;
    boost::shared_ptr<Immediate> immediate_;
    boost::shared_ptr<style::Theme> theme_;
};

};  // namespace v3d::ui::shell
