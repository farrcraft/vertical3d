/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "ComponentRenderer.h"

#include "../asset/Manager.h"
#include "../font/TextureFontCache.h"
#include "../font/TextureTextBuffer.h"
#include "../log/Logger.h"
#include "../render/realtime/Canvas.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::vulkan {
class QuadRenderer;
};  // namespace v3d::render::realtime::vulkan

namespace v3d::ui {

/**
 * One font, packed into one atlas, and the text drawn with it.
 *
 * A glyph is the batched quad of ADR-0005 sampling that atlas, so a line of text joins
 * whatever canvas the app is already filling and costs the frame no pass and no draw of
 * its own.
 *
 * measure() and write() are the pair ComponentRenderer is built from. Per ADR-0019 that
 * class takes them as callbacks and names no font type, so drawing a ui costs it no device
 * and this class is what supplies one.
 **/
class TextRenderer {
 public:
    /**
     * Printable ascii - the glyphs an app draws unless it names its own set.
     **/
    static const wchar_t* const ascii;

    /**
     * The font in the shared data directory, which every app in the tree draws with.
     **/
    static const char* const defaultFont;

    /**
     * Load a font, pack its glyphs into an atlas and upload it.
     *
     * The atlas goes to the device once, here, so every glyph that will ever be drawn has
     * to be in charcodes. A font that cannot be loaded leaves this measuring zero and
     * drawing nothing - the loader has already said which file it could not read - so a
     * missing font costs the app its labels rather than its frame.
     *
     * @param quads the renderer the atlas is uploaded through - Engine3D::quads()
     * @param size the size the font is rasterized at. Nothing scales a glyph, so this is
     *        also the size everything is drawn at
     * @param font the asset to load, resolved against the manager's path
     * @param charcodes the glyphs to pack
     **/
    TextRenderer(const boost::shared_ptr<v3d::asset::Manager>& assetManager,
        const boost::shared_ptr<v3d::log::Logger>& logger,
        const boost::shared_ptr<v3d::render::realtime::vulkan::QuadRenderer>& quads,
        float size,
        const std::string& font = defaultFont,
        const wchar_t* charcodes = ascii);

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    /**
     * @return whether a font was loaded, which is what decides if anything is drawn
     **/
    bool loaded() const noexcept;

    /**
     * @return the size the font was rasterized at, for laying out around a line of it
     **/
    float size() const noexcept;

    /**
     * @return how wide a string is when it is drawn, in pixels
     **/
    float width(const std::string& text) const;

    /**
     * Lay a string out at the pen and append its glyphs to a canvas.
     *
     * @param pen where the baseline of the first glyph goes
     **/
    void draw(v3d::render::realtime::Canvas* canvas, const std::string& text, const glm::vec2& pen, const glm::vec4& colour);

    /**
     * @return width() as the callback ComponentRenderer takes
     **/
    ComponentRenderer::Measure measure() const;

    /**
     * @return draw() as the callback ComponentRenderer takes, against one canvas
     *
     * The canvas is held by the returned callback rather than copied, so it has to outlive
     * the ComponentRenderer it is given to.
     **/
    ComponentRenderer::Write write(v3d::render::realtime::Canvas* canvas);

 private:
    boost::shared_ptr<v3d::font::TextureFontCache> cache_;
    boost::shared_ptr<v3d::font::TextureTextBuffer> buffer_;
    v3d::font::TextureTextBuffer::Markup markup_;
    v3d::render::realtime::TextureHandle atlas_;
    float size_;
};

};  // namespace v3d::ui
