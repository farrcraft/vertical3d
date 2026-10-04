/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Canvas.h>
#include <api/ui/Engine.h>
#include <api/ui/Immediate.h>
#include <api/ui/paint/ComponentRenderer.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/TextRenderer.h>
#include <api/ui/shell/StatisticsOverlay.h>

#include <functional>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
class Engine3D;
};  // namespace v3d::render::realtime

namespace v3d::ui::shell {

/**
 * The text an app draws with, the renderers over it, and the canvas they fill, built once
 * over the renderer the app already has - ADR-0074.
 *
 * What is drawn into the canvas, and which pass it is submitted to, stay the app's. So do
 * the passes themselves: begin() begins the frame and sizes the canvas, and the app fills it
 * and submits it where it likes.
 *
 * Everything that closes over the font's size is rebuilt by scale(), so a component renderer
 * or an immediate layer this hands out is one to draw with now and not to keep.
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
        Dress dress;              /**< the dressing; a line height of 1.4 sizes when empty **/
        bool statistics { true }; /**< build a statistics overlay **/
        bool immediate { false }; /**< build an immediate layer over the same text **/
    };

    /**
     * @param engine the renderer the atlas is uploaded through and frames are begun on. Not
     *        owned, and it must outlive this. Null builds everything with no device, which
     *        uploads nothing and begins no frame
     **/
    Screen(v3d::render::realtime::Engine3D* engine, const boost::shared_ptr<v3d::asset::Manager>& assets,
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
     *         there is no renderer. The frame has already been presented empty, so the app
     *         draws nothing and returns
     **/
    bool begin();

    /**
     * @return whether the last begin() found the frame a different size from the one before,
     *         which the first one always does. This is when an app resizes what is its own
     **/
    bool resized() const noexcept;

    /**
     * Draw text larger or smaller, rebuilding what closes over its size. The dressing is
     * filled in again for the new size, and the theme is given back to what is rebuilt.
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
     * Dress the component renderer and the immediate layer from a theme, and keep it for
     * what scale() rebuilds.
     **/
    void theme(const boost::shared_ptr<style::Theme>& theme);

    /**
     * Draw a ui, then the statistics over it, into the canvas. Either may be missing.
     *
     * @param ui what to draw, or null for nothing but the statistics
     **/
    void draw(const Engine* ui, const StatisticsOverlay::Sample& statistics);

    v3d::render::realtime::Canvas& canvas() noexcept;
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

    v3d::render::realtime::Engine3D* engine_;
    Options options_;
    float scale_;
    bool resized_;
    v3d::render::realtime::Canvas canvas_;
    boost::shared_ptr<paint::TextRenderer> text_;
    boost::shared_ptr<paint::ComponentRenderer> components_;
    boost::shared_ptr<StatisticsOverlay> statistics_;
    boost::shared_ptr<Immediate> immediate_;
    boost::shared_ptr<style::Theme> theme_;
};

};  // namespace v3d::ui::shell
