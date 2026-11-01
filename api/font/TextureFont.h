/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/TextureAtlas.h>
#include <api/log/Logger.h>

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>
#include <string_view>

#include <glm/glm.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::font::internal {
class Freetype;
};  // namespace v3d::font::internal

namespace v3d::font {

class TextureFont {
 public:
    typedef enum : std::uint8_t {
        OUTLINE_TYPE_NONE = 0,
        OUTLINE_TYPE_LINE = 1,
        OUTLINE_TYPE_INNER = 2,
        OUTLINE_TYPE_OUTER = 3
    } OutlineType;

    /**
     * The charcode of the opaque white square that lines and backgrounds are drawn with. It
     * is not a code point, so no text names it.
     **/
    static constexpr char32_t SOLID = 0xFFFFFFFF;

    typedef struct {
        unsigned int id_;
        char32_t charcode_;
        unsigned int width_;
        unsigned int height_;
        glm::ivec2 offset_;
        glm::vec2 advance_;
        glm::vec2 st_[2];
        OutlineType outline_;
        float outlineThickness_;
    } Glyph;

    /**
     * @param size the size the face is rasterized at. For a distance field this is the
     *        base size, and every drawn size is a ratio of it
     * @param spread how far either side of an edge the distance field runs, in pixels at
     *        the base size, or zero to rasterize coverage. It decides how far a glyph can
     *        be scaled up before its edge softens, and adds twice its value to each axis of
     *        every glyph in the atlas
     **/
    TextureFont(const std::string& filename, float size, const boost::shared_ptr<v3d::log::Logger> & logger,
        unsigned int spread = 0);

    /**
     * The glyph for a code point, packed the first time it is asked for, or null when it
     * cannot be. SOLID is the white square, made the first time it is asked for, and null when
     * the atlas has no room for it.
     **/
    boost::shared_ptr<Glyph> glyph(char32_t charcode);

    /**
     * The glyph already packed for a charcode, or null. A charcode this face lacks is looked
     * up in the fallback, when there is one.
     *
     * glyph() rasterizes and packs a charcode it has not seen. This never does, so the atlas
     * is unchanged. A caller that has uploaded the atlas looks glyphs up through this.
     **/
    boost::shared_ptr<Glyph> packed(char32_t charcode) const;

    /**
     **/
    void atlas(const boost::shared_ptr<v3d::image::TextureAtlas>& atlas);

    /**
     * Another face for the code points this one lacks.
     *
     * Packing gives the fallback each code point this face has no glyph for, and packed()
     * looks there for it, so text that mixes the two lays out as one font. The fallback packs
     * into its own atlas, which has to be this face's, and is rasterized at its own size,
     * which has to match this face's. Several faces may share one fallback, and a code point
     * is packed into it once.
     *
     * Without a fallback, a code point this face lacks is packed as the face's missing glyph.
     **/
    void fallback(const boost::shared_ptr<TextureFont>& font);

    /**
     * Rasterize each character of a UTF-8 string and pack it into the atlas.
     *
     * A glyph that does not fit is counted rather than drawn. Because of the count, this
     * can fail after having packed most of what it was given. Text drawn with a partly
     * packed font is missing characters and measures short, so the layout around it is
     * wrong too. A caller that ignores the return value gets both silently.
     *
     * @return whether every charcode was packed
     **/
    bool loadGlyphs(std::string_view utf8);

    /**
     * How wide a UTF-8 string is when it is laid out at a size, in pixels.
     *
     * A code point with no packed glyph adds nothing, the same as the layout draws nothing for
     * it. Nothing is packed.
     *
     * @param size the size it is laid out at. Zero or less means the font's own size
     **/
    float width(std::string_view utf8, float size = 0.0F) const;

    float size() const;

    /**
     * @return the distance field spread, or zero when the glyphs are coverage
     **/
    unsigned int spread() const;

    std::string filename() const;

    float ascender() const;
    float descender() const;
    float height() const;
    float linegap() const;
    float underlinePosition() const;
    float underlineThickness() const;

 protected:
    static boost::shared_ptr<Glyph> createGlyph();

 private:
    /**
     * Rasterize each code point and pack it, as loadGlyphs() does.
     **/
    bool pack(const std::u32string& charcodes);

    /**
     * Rasterize one code point and pack it. The face has to be loaded, and is released when
     * this fails.
     *
     * @param sdf whether the glyph is rendered as a distance field
     * @param missed counts the glyph when it does not fit the atlas, which is not a failure
     * @return false when FreeType fails to load, render or stroke the glyph
     **/
    bool packGlyph(char32_t charcode, bool sdf, unsigned int* missed);

    /**
     * Divide code points between this face and its fallback, leaving out any this face has
     * already packed. The face has to be loaded.
     *
     * @param mine what this face packs, including a code point it lacks when it has no fallback
     * @param lacking what the fallback packs
     **/
    void divide(const std::u32string& charcodes, std::u32string* mine, std::u32string* lacking) const;

    /**
     * The glyph this face packed for a charcode, not looking in the fallback.
     **/
    boost::shared_ptr<Glyph> own(char32_t charcode) const;

    /** The number of weights in the filter applied across subpixels. **/
    static constexpr std::size_t LCD_FILTER_TAPS = 5;

    std::vector<boost::shared_ptr<Glyph> > glyphs_;
    boost::shared_ptr<v3d::image::TextureAtlas> atlas_;
    boost::shared_ptr<TextureFont> fallback_;
    std::string filename_;
    float size_;
    unsigned int spread_;
    int hinting_;
    OutlineType outline_;
    float outlineThickness_;
    int lcdFiltering_;
    unsigned char lcdWeights_[LCD_FILTER_TAPS];
    float height_;
    float linegap_;
    float ascender_;
    float descender_;
    float underlinePosition_;
    float underlineThickness_;

    boost::shared_ptr<internal::Freetype> freetype_;
    boost::shared_ptr<v3d::log::Logger> logger_;
};
};  // namespace v3d::font
