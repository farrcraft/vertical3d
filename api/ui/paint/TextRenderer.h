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
#include <api/render/realtime/canvas/Canvas.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "Text.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::paint {

/**
 * One typeface in two weights, packed into one atlas, and the text drawn with it.
 *
 * The regular and the semibold face each pack the charset. A code point either lacks is packed
 * from a third face, the fallback, which both share, so a string mixing the faces lays out as
 * one font.
 *
 * A glyph is a batched quad sampling that atlas, so a line of text joins whatever canvas
 * the app is already filling, and costs the frame no pass and no draw of its own.
 *
 * measure() and write() return the callback pair that ComponentRenderer and Immediate take.
 * Those classes name no font type, so they draw a ui without a device. This class supplies
 * the font.
 **/
class TextRenderer {
 public:
    /**
     * How an atlas image becomes a texture the canvas can name.
     *
     * The only step in this class that needs a device, so the app passes it in. Everything
     * else here is cpu side: the cache packs into an image::TextureAtlas and the buffer
     * lays a string out. Because the upload is a callback, an app that draws this canvas
     * with a renderer of its own can use the class. With WindowRenderer, the callback is
     * engine->textures()->texture(image).
     **/
    typedef std::function<v3d::render::realtime::TextureHandle(
        const boost::shared_ptr<v3d::image::Image>&)> Upload;

    /**
     * Printable ascii, as UTF-8 - the glyphs an app draws unless it names its own set.
     **/
    static const char* const ascii;

    /**
     * The regular face in the shared data directory, used when the caller names no other.
     **/
    static const char* const defaultFont;

    /**
     * The semibold face in the shared data directory, used when the caller names no other.
     **/
    static const char* const semiBoldFont;

    /**
     * The face in the shared data directory that the code points the other two lack are
     * drawn from, used when the caller names no other.
     **/
    static const char* const fallbackFont;

    /**
     * The files a renderer loads its faces from, each resolved against the asset manager's
     * path. An empty semibold draws semibold text in the regular face, and an empty fallback
     * packs a code point the faces lack as their missing glyph.
     **/
    struct Faces final {
        /**
         * The faces in the shared data directory.
         **/
        Faces();

        std::string regular;
        std::string semiBold;
        std::string fallback;
    };

    /**
     * The size the atlas is rasterized at, which every drawn size is a ratio of.
     *
     * Large enough that a ui mostly scales the glyphs down, because a distance field keeps
     * detail when scaled down better than when scaled up. Small enough that printable ascii
     * and its spread still fit defaultAtlas.
     **/
    static const float baseSize;

    /**
     * How far either side of an edge the distance field runs, in pixels at baseSize.
     *
     * The spread sets how far a glyph can be scaled before its edge softens. Each glyph
     * takes twice the spread of extra atlas room in each axis. 8 is freetype's own default,
     * and holds an edge well past the sizes a ui draws at.
     **/
    static const unsigned int defaultSpread;

    /**
     * The side of the square the glyphs are packed into. Printable ascii in both weights at
     * baseSize with defaultSpread fits this, so a larger charset, a larger base or a wider
     * spread needs a larger atlas, passed to the constructor. A font that does not fit is
     * logged.
     **/
    static const unsigned int defaultAtlas;

    /**
     * Load the faces, pack their glyphs into an atlas and upload it.
     *
     * The atlas is uploaded once, here, so every glyph that will ever be drawn has to be in
     * charset. A regular face that cannot be loaded, and glyphs that do not all fit the
     * atlas, both leave this measuring zero and drawing nothing, and the log says which. The
     * app then loses its labels but still draws its frame. A semibold face that cannot be
     * loaded draws semibold text in the regular face, and a fallback that cannot be loaded
     * leaves the missing glyphs in place, and the log says so.
     *
     * @param upload what puts the packed atlas on the device and names it
     * @param size the size the font is rasterized at. The glyphs are distance fields, so
     *        this is the base every drawn size is a ratio of rather than the only size
     * @param faces the assets to load
     * @param charset the glyphs to pack, as a UTF-8 string
     * @param spread the distance field spread, or zero to pack coverage glyphs that only
     *        draw correctly at size
     * @param atlas the width and height of the atlas to pack into, which has to hold every
     *        glyph at size
     **/
    TextRenderer(const boost::shared_ptr<v3d::asset::Manager>& assetManager,
        const boost::shared_ptr<v3d::log::Logger>& logger,
        const Upload& upload,
        float size = baseSize,
        const Faces& faces = Faces(),
        std::string_view charset = ascii,
        unsigned int spread = defaultSpread,
        const glm::uvec2& atlas = glm::uvec2(defaultAtlas));

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    /**
     * Whether a font was loaded with all of its glyphs packed. Nothing is measured or drawn
     * when it was not.
     *
     * A partly packed atlas counts as not loaded. Drawing what did fit would put text on
     * screen with characters missing and measure it short, so the ui laid out around that
     * measure would be wrong too, with no error.
     **/
    bool loaded() const noexcept;

    /**
     * @return the size the atlas was rasterized at, which a drawn size is a ratio of
     **/
    float size() const noexcept;

    /**
     * How wide a string is when it is drawn at a size, in pixels.
     *
     * The string is UTF-8. A code point that is not in the packed set adds nothing, the same
     * as draw() draws nothing for it.
     *
     * @param size the size it will be drawn at, defaulting to the one the atlas holds
     * @param weight the face it will be drawn in
     **/
    float width(std::string_view text, float size = 0.0F, Weight weight = Weight::Regular) const;

    /**
     * Lay a string out at the pen and append its glyphs to a canvas.
     *
     * The string is UTF-8. A code point that is not in the packed set draws nothing and
     * advances the pen by nothing. The atlas is never added to after the upload, so a glyph
     * packed later would not be on the device.
     *
     * @param pen where the baseline of the first glyph goes
     * @param size the size to draw at, defaulting to the one the atlas holds
     * @param weight the face to draw in
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, std::string_view text, const glm::vec2& pen, const glm::vec4& colour,
        float size = 0.0F, Weight weight = Weight::Regular);

    /**
     * @return width() at a size and a weight, as the callback ComponentRenderer and Immediate
     *         take
     **/
    Measure measure(float size = 0.0F, Weight weight = Weight::Regular) const;

    /**
     * @return draw() at a size into one canvas, as the callback ComponentRenderer and
     *         Immediate take
     *
     * The returned callback holds the canvas pointer, so the canvas must outlive the
     * callback.
     *
     * The callback pair names no font type, so the size is captured here instead of being
     * passed with each string. A ui at one size and a heading at another are two callback
     * pairs from one TextRenderer, and one atlas serves both.
     **/
    Write write(v3d::render::realtime::canvas::Canvas* canvas, float size = 0.0F, Weight weight = Weight::Regular);

 private:
    /**
     * @return the face text of a weight is drawn in
     **/
    const boost::shared_ptr<v3d::font::TextureFont>& face(Weight weight) const noexcept;

    boost::shared_ptr<v3d::font::TextureFontCache> cache_;
    boost::shared_ptr<v3d::font::TextureTextBuffer> buffer_;
    v3d::font::TextureTextBuffer::Markup markup_;
    boost::shared_ptr<v3d::font::TextureFont> regular_;
    boost::shared_ptr<v3d::font::TextureFont> semiBold_;
    v3d::render::realtime::TextureHandle atlas_;
    float size_;
    // what draw() hands the canvas, kept between draws so a frame of labels does not allocate
    std::vector<v3d::render::realtime::canvas::Canvas::Vertex> vertices_;
    std::vector<uint32_t> indices_;
};

};  // namespace v3d::ui::paint
