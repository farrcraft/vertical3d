/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::font::face {

class Face;

};  // namespace v3d::font::face

namespace v3d::font::glyph {


/**
 * How a glyph's pixels are made.
 **/
enum class Mode : std::uint8_t {
    Coverage,  /**< how much of each pixel the glyph covers **/
    Distance,  /**< each pixel's distance from the glyph's edge **/
    Colour     /**< each pixel's colour, from the face's colour tables **/
};

/**
 * What tells one stored glyph from another.
 **/
struct Key final {
    const face::Face* face = nullptr;
    unsigned int index = 0;                /**< the glyph's index in its face **/
    float size = 0.0F;                     /**< in pixels **/
    std::uint8_t subpixel = 0;             /**< the step of a pixel its pen was at, horizontally **/
    Mode mode = Mode::Coverage;
    std::uint8_t spread = 0;               /**< how far a distance field reaches from the edge, in pixels. Coverage leaves it 0 **/
    std::uint16_t palette = 0;             /**< a colour glyph's palette. Other modes leave it 0 **/
    std::uint32_t foreground = 0;          /**< the text's colour, which a colour glyph's layers may take, packed by packColour(). Other modes leave it 0 **/
    std::vector<std::int32_t> coordinates; /**< a variable font's axes, normalized, or none **/

    auto operator<=>(const Key& other) const = default;
};

/**
 * A glyph's pixels, in rows, before it is stored. A pixel is one byte, or for a colour glyph
 * four: red, green and blue, unpremultiplied, then alpha.
 **/
struct Bitmap final {
    glm::uvec2 size{ 0 };
    glm::ivec2 bearing{ 0 };            /**< the top left pixel from the pen, y downwards **/
    std::vector<unsigned char> pixels;  /**< size.x * size.y * channels of them **/
    std::uint8_t channels = 1;
};

/**
 * Where a glyph is stored.
 **/
struct StoredGlyph final {
    std::size_t page = 0;
    glm::uvec2 position{ 0 };  /**< its top left texel in the page **/
    glm::uvec2 size{ 0 };
    glm::ivec2 bearing{ 0 };
};

/**
 * A stretch of a page whose pixels changed and have to reach the device.
 **/
struct Region final {
    std::size_t page = 0;
    glm::uvec2 position{ 0 };
    glm::uvec2 size{ 0 };
};

/**
 * The glyphs text is drawn from, made the first time each is drawn and packed into pages.
 *
 * Each page is a square of pixels of one byte or four, cut into square plots, and each plot
 * holds glyphs on shelves. A glyph goes into a plot with room, then an empty plot, then a new
 * page up to the page limit. With none of those, the plot used longest ago is emptied and used
 * again: every glyph in it is forgotten, and made again the next time it is drawn.
 *
 * A plot holding a glyph used in the current frame is never emptied, because the frame being
 * built draws from it. When every plot is in that use, a page is added past the limit. A page
 * past the limit is let go once a frame has begun that the frame before used nothing in it.
 *
 * A key names its face by its address, so a face the store holds glyphs from stays open while
 * the store is used. The store keeps every page's pixels. regions() gives the plots changed
 * since it was last asked, for a renderer to copy to the device, and a page added is changed
 * whole. A store is used from one thread.
 **/
class Store final {
 public:
    /**
     * The shape of the pages.
     **/
    struct Spec final {
        std::uint32_t pageSize = 1024;   /**< each page's width and height, in pixels **/
        std::uint32_t plotSize = 256;    /**< each plot's, which divides the page's **/
        std::size_t pages = 4;           /**< the pages kept when no frame needs more **/
        std::uint32_t padding = 1;       /**< the empty pixels around each glyph **/
        std::uint8_t channels = 1;       /**< the bytes in a pixel: 1, or 4 for colour. A bitmap of another count is not stored **/
    };

    /**
     * Makes a glyph's pixels for its key.
     **/
    using Rasterizer = std::function<Bitmap(const Key&)>;

    /**
     * A store with the default pages.
     **/
    Store();

    explicit Store(const Spec& spec);

    /**
     * Start a frame. A plot used in the frame before is no longer kept for it.
     **/
    void beginFrame();

    /**
     * @return where a glyph is stored, made and packed the first time it is asked for, and
     *         marked as used in the current frame. A glyph with no pixels is stored with no
     *         size. A glyph wider or taller than a plot with its padding, a bitmap whose
     *         pixels do not match its size, or one with other channels than the pages, is not
     *         stored
     **/
    std::optional<StoredGlyph> glyph(const Key& key, const Rasterizer& rasterize);

    /**
     * @return how many pages there are
     **/
    std::size_t pages() const noexcept;

    /**
     * @return a page's pixels, in rows of the page's size, each pixel its channels
     **/
    const std::vector<unsigned char>& pixels(std::size_t page) const;

    /**
     * @return the regions changed since this was last called, each a whole plot. A page let go
     *         since is not among them
     **/
    std::vector<Region> regions();

    /**
     * @return how many glyphs have been made rather than found
     **/
    std::size_t made() const noexcept;

    const Spec& spec() const noexcept;

 private:
    /**
     * A row of a plot, as tall as the first glyph put on it.
     **/
    struct Shelf {
        std::uint32_t y = 0;
        std::uint32_t height = 0;
        std::uint32_t x = 0;  /**< where the next glyph goes **/
    };

    /**
     * A square of a page, which holds glyphs on shelves and is emptied as one.
     **/
    struct Plot {
        std::size_t page = 0;
        glm::uvec2 origin{ 0 };
        std::vector<Shelf> shelves;
        std::vector<Key> keys;  /**< the glyphs in it **/
        std::uint64_t used = 0;      /**< the last frame a glyph in it was asked for **/
        bool dirty = false;
    };

    struct Entry {
        StoredGlyph stored;
        std::size_t plot = 0;
    };

    /**
     * @return where a glyph of a size fits in a plot, with its padding, or nothing
     **/
    std::optional<glm::uvec2> place(Plot* plot, const glm::uvec2& size) const;

    /**
     * @return the plot a glyph of a size goes into, making room as the class says, or nothing
     *         when it is larger than a plot
     **/
    std::optional<std::pair<std::size_t, glm::uvec2>> room(const glm::uvec2& size);

    /**
     * Add a page of empty plots, every one of them changed.
     **/
    void addPage();

    /**
     * Forget every glyph in a plot and clear its pixels.
     **/
    void empty(std::size_t plot);

    /**
     * Copy a bitmap's pixels into a page, its top left texel at a position.
     **/
    void blit(const Bitmap& bitmap, std::size_t page, const glm::uvec2& position);

    Spec spec_;
    std::uint64_t frame_ = 1;
    std::size_t made_ = 0;
    std::vector<std::vector<unsigned char>> pages_;
    std::vector<Plot> plots_;
    std::map<Key, Entry> entries_;
};

};  // namespace v3d::font::glyph
