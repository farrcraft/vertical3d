/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/font/TextureFont.h>
#include <api/font/TextureFontCache.h>
#include <api/font/TextureTextBuffer.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Canvas.h>

#include <cstddef>
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
 * A glyph is a batched quad sampling that atlas, so a line of text joins
 * whatever canvas the app is already filling and costs the frame no pass and no draw of
 * its own.
 *
 * measure() and write() are the callback pair ComponentRenderer is built from. That class
 * names no font type, so it draws a ui without a device, and this class supplies the font.
 **/
class TextRenderer {
 public:
    /**
     * How an atlas image becomes a texture the canvas can name.
     *
     * The only thing in this class that needs a device, so it is handed in. Everything
     * else here is cpu side: the cache packs into an image::TextureAtlas and the buffer
     * lays a string out. Taking the upload as a callback lets an app that draws this
     * canvas with a renderer of its own use the class. Engine3D's is quads->texture(image).
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
     * a wider spread needs more. That is why it is an argument, and a font that does not
     * fit is logged.
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
     * @param size the size the font is rasterized at. The glyphs are distance fields, so
     *        this is the base every drawn size is a ratio of rather than the only size
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
     * Whether there is a font with all of its glyphs behind this, which decides
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
     * The code points a UTF-8 string holds, one per character.
     *
     * A byte that does not begin a well formed sequence becomes U+FFFD, the replacement
     * character, and decoding resumes at the next byte. Overlong forms and surrogates count
     * as malformed.
     **/
    static std::u32string decode(std::string_view utf8);

    /**
     * How wide a string is when it is drawn at a size, in pixels.
     *
     * The string is UTF-8. A code point that is not in the packed set adds nothing, the same
     * as draw() draws nothing for it.
     *
     * @param size the size it will be drawn at, defaulting to the one the atlas holds
     **/
    float width(std::string_view text, float size = 0.0f) const;

    /**
     * Lay a string out at the pen and append its glyphs to a canvas.
     *
     * The string is UTF-8. A code point that is not in the packed set draws nothing and
     * advances the pen by nothing. The atlas is never added to after the upload, so a glyph
     * packed later would not be on the device.
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
     * The pair names no font type, so the size is closed over here rather
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

    /**
     * Decode the code point that starts at *index and move *index past it. Malformed input
     * yields U+FFFD and moves one byte, as decode() describes.
     **/
    static char32_t next(std::string_view utf8, std::size_t* index);

    /**
     * The packed glyph for a code point, or null. This never packs one, because the atlas
     * is already on the device.
     **/
    boost::shared_ptr<v3d::font::TextureFont::Glyph> packed(char32_t point) const;

    boost::shared_ptr<v3d::font::TextureFontCache> cache_;
    boost::shared_ptr<v3d::font::TextureTextBuffer> buffer_;
    v3d::font::TextureTextBuffer::Markup markup_;
    v3d::render::realtime::TextureHandle atlas_;
    float size_;
};

};  // namespace v3d::ui::paint
