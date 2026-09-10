/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/TextureAtlas.h>
#include <api/log/Logger.h>

#include <vector>
#include <string>

#include <glm/glm.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::font {

class TextureFont {
 public:
    typedef struct {
        wchar_t charcode_;
        float kerning_;
    } Kerning;

    typedef enum {
        OUTLINE_TYPE_NONE = 0,
        OUTLINE_TYPE_LINE = 1,
        OUTLINE_TYPE_INNER = 2,
        OUTLINE_TYPE_OUTER = 3
    } OutlineType;

    typedef struct {
        unsigned int id_;
        wchar_t charcode_;
        unsigned int width_;
        unsigned int height_;
        glm::ivec2 offset_;
        glm::vec2 advance_;
        glm::vec2 st_[2];
        std::vector<Kerning> kerning_;
        OutlineType outline_;
        float outlineThickness_;
    } Glyph;

    /**
     * @param size the size the face is rasterized at, which for a distance field is the
     *        one base size every drawn size is a ratio of - ADR-0036
     * @param spread how far either side of an edge the distance field runs, in pixels at
     *        the base size, or zero to rasterize coverage as this always used to. It is
     *        what decides how far a glyph can be scaled up before its edge softens, and
     *        it costs the atlas twice itself in each axis of every glyph
     **/
    TextureFont(const std::string& filename, float size, const boost::shared_ptr<v3d::log::Logger> & logger,
        unsigned int spread = 0);

    boost::shared_ptr<Glyph> glyph(wchar_t charcode);
    static float kerning(boost::shared_ptr<Glyph> glyph, wchar_t charcode);

    /**
     **/
    void atlas(boost::shared_ptr<v3d::image::TextureAtlas> atlas);

    /**
     * Rasterize each charcode and pack it into the atlas.
     *
     * A glyph that does not fit is counted rather than drawn, and the count is why this
     * can fail after having packed most of what it was given: text drawn with a partly
     * packed font is missing characters and measures short, so the layout around it is
     * wrong too. A caller that ignores the answer gets both silently.
     *
     * @return whether every charcode was packed
     **/
    bool loadGlyphs(const wchar_t * charcodes);

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
    void generateKerning();
    static boost::shared_ptr<Glyph> createGlyph();

 private:
    std::vector<boost::shared_ptr<Glyph> > glyphs_;
    boost::shared_ptr<v3d::image::TextureAtlas> atlas_;
    std::string filename_;
    float size_;
    unsigned int spread_;
    int hinting_;
    OutlineType outline_;
    float outlineThickness_;
    int lcdFiltering_;
    unsigned char lcdWeights_[5];
    float height_;
    float linegap_;
    float ascender_;
    float descender_;
    float underlinePosition_;
    float underlineThickness_;

    class Freetype;
    boost::shared_ptr<Freetype> freetype_;
    boost::shared_ptr<v3d::log::Logger> logger_;
};
};  // namespace v3d::font
