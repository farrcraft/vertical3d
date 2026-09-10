/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/font/TextureFontCache.h>
#include <api/font/TextureTextBuffer.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Canvas.h>

#include <functional>
#include <string>
#include <string_view>

#include "Text.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::paint {

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
     * How an atlas image becomes a texture the canvas can name.
     *
     * The one thing in this class that needs a device, so it is the one thing handed in.
     * Everything else here is cpu side - the cache packs into an image::TextureAtlas and
     * the buffer lays a string out - and taking the upload as a callback is what lets an
     * app drawing this canvas with a renderer of its own use the class rather than copy
     * it. Engine3D's is quads->texture(image).
     **/
    typedef std::function<v3d::render::realtime::TextureHandle(
        const boost::shared_ptr<v3d::image::Image>&)> Upload;

    /**
     * Printable ascii - the glyphs an app draws unless it names its own set.
     **/
    static const wchar_t* const ascii;

    /**
     * The font in the shared data directory, which every app in the tree draws with.
     **/
    static const char* const defaultFont;

    /**
     * The size the atlas is rasterized at, which every drawn size is a ratio of.
     *
     * Large enough that scaling down is what a ui mostly does, since a distance field
     * carries detail down better than it invents it going up, and small enough that
     * printable ascii and its spread still fit defaultAtlas.
     **/
    static const float baseSize;

    /**
     * How far either side of an edge the distance field runs, in pixels at baseSize.
     *
     * This is what sets how far a glyph can be scaled before its edge softens, and it is
     * charged to the atlas twice over in each axis of every glyph. 8 is freetype's own
     * default and holds an edge well past the sizes a ui draws at.
     **/
    static const unsigned int defaultSpread;

    /**
     * The square the glyphs are packed into. Printable ascii at baseSize with
     * defaultSpread fits this with little to spare, so a larger charset, a larger base or
     * a wider spread wants more - which is why it is an argument rather than a constant
     * in the body, and why a font that does not fit now says so.
     **/
    static const unsigned int defaultAtlas;

    /**
     * Load a font, pack its glyphs into an atlas and upload it.
     *
     * The atlas goes to the device once, here, so every glyph that will ever be drawn has
     * to be in charcodes. A font that cannot be loaded, and one whose glyphs do not all
     * fit the atlas, both leave this measuring zero and drawing nothing - the log already
     * says which - so either costs the app its labels rather than its frame.
     *
     * @param upload what puts the packed atlas on the device and names it
     * @param size the size the font is rasterized at, which per ADR-0036 is the base every
     *        drawn size is a ratio of rather than the only size available
     * @param font the asset to load, resolved against the manager's path
     * @param charcodes the glyphs to pack
     * @param spread the distance field spread, or zero to pack coverage glyphs that only
     *        draw correctly at size
     * @param atlasWidth the atlas to pack into, which has to hold every charcode at size
     * @param atlasHeight likewise
     **/
    TextRenderer(const boost::shared_ptr<v3d::asset::Manager>& assetManager,
        const boost::shared_ptr<v3d::log::Logger>& logger,
        const Upload& upload,
        float size = baseSize,
        const std::string& font = defaultFont,
        const wchar_t* charcodes = ascii,
        unsigned int spread = defaultSpread,
        unsigned int atlasWidth = defaultAtlas,
        unsigned int atlasHeight = defaultAtlas);

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    /**
     * Whether there is a font with all of its glyphs behind this, which is what decides
     * whether anything is measured or drawn.
     *
     * A partly packed atlas counts as not loaded. Drawing what did fit would put text on
     * screen with characters missing and measure it short, so the ui laid out around that
     * measure would be wrong too, and none of it would say so.
     **/
    bool loaded() const noexcept;

    /**
     * @return the size the atlas was rasterized at, which a drawn size is a ratio of
     **/
    float size() const noexcept;

    /**
     * How wide a string is when it is drawn at a size, in pixels.
     *
     * @param size the size it will be drawn at, defaulting to the one the atlas holds
     **/
    float width(std::string_view text, float size = 0.0f) const;

    /**
     * Lay a string out at the pen and append its glyphs to a canvas.
     *
     * @param pen where the baseline of the first glyph goes
     * @param size the size to draw at, defaulting to the one the atlas holds
     **/
    void draw(v3d::render::realtime::Canvas* canvas, std::string_view text, const glm::vec2& pen, const glm::vec4& colour,
        float size = 0.0f);

    /**
     * @return width() at a size, as the callback both ways of writing a ui take
     **/
    Measure measure(float size = 0.0f) const;

    /**
     * @return draw() at a size, as the callback both ways of writing a ui take, against one canvas
     *
     * The canvas is held by the returned callback rather than copied, so it has to outlive
     * whatever it is given to.
     *
     * Per ADR-0019 the pair names no font type, so the size is closed over here rather
     * than travelling with each string: a ui at one size and a heading at another are two
     * callback pairs from one TextRenderer, and one atlas serves both.
     **/
    Write write(v3d::render::realtime::Canvas* canvas, float size = 0.0f);

 private:
    /**
     * How much a requested size scales what the atlas holds. Zero, and anything else that
     * is not a size, means the base - so a caller that does not care never scales.
     **/
    float ratio(float size) const noexcept;

    boost::shared_ptr<v3d::font::TextureFontCache> cache_;
    boost::shared_ptr<v3d::font::TextureTextBuffer> buffer_;
    v3d::font::TextureTextBuffer::Markup markup_;
    v3d::render::realtime::TextureHandle atlas_;
    float size_;
};

};  // namespace v3d::ui::paint
