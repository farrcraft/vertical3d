/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "SpriteRegion.h"

#include <map>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::config {

/**
 * One image and a table of names over rectangles in it.
 *
 * The image is named rather than loaded: an app resolves it through its own asset manager
 * and renderer, as it does the images a theme names.
 *
 * **The rectangles are in pixels and the sheet states its own size**, so uv() divides one by
 * the other. An author reads a sprite sheet in pixels and a shader needs a fraction, and
 * pixels in the document can be checked against the image. Because the sheet states its size,
 * the document stays valid if the image is rescaled: the pixels are relative to the size
 * written here, not to the file on disk.
 **/
class SpriteSheet final {
 public:
    SpriteSheet();

    /**
     * Build a sheet to put sprites in, for a packer emitting a document rather than a
     * reader filling one out.
     *
     * The size is given here and not per sprite because place() measures each region
     * against it. A sheet built this way therefore refuses exactly what a sheet read from a
     * document refuses, and a tool cannot emit a region the reader will drop.
     **/
    SpriteSheet(const std::string& name, const std::string& image, int width, int height);

    /**
     * @return what the sheet is called, as whatever names one names it
     **/
    const std::string& name() const noexcept;

    /**
     * @return the image the regions are cut from, for the app to resolve
     **/
    const std::string& image() const noexcept;

    /**
     * @return the size the document says the sheet is, which uv() divides by
     **/
    int width() const noexcept;
    int height() const noexcept;

    /**
     * @return whether a sprite of that name is in the sheet
     **/
    bool has(const std::string& sprite) const;

    /**
     * @return the region, or an empty one when the sheet does not hold that sprite
     **/
    SpriteRegion get(const std::string& sprite) const;

    /**
     * @return the sprite names, in the order the document listed them
     **/
    const std::vector<std::string>& sprites() const noexcept;

    /**
     * One sprite's region as the uv pair a canvas takes.
     *
     * @param sprite the name to look up
     * @param uv0 filled with the corner at the region's minimum
     * @param uv1 filled with the corner at its maximum
     * @return false when the sheet does not hold that sprite, or states no size to divide
     *         by, leaving both outputs alone
     **/
    bool uv(const std::string& sprite, glm::vec2* uv0, glm::vec2* uv1) const;

    /**
     * Put one sprite in the sheet. A region running off the sheet is refused, because it
     * would give a uv outside 0..1, which samples according to the wrap mode instead of
     * failing visibly.
     *
     * @return whether the region is one this sheet can hold. A name already in the sheet is
     *         kept as it was and is not an error
     **/
    bool place(const std::string& sprite, const SpriteRegion& region);

 private:
    friend class SpriteSheets;

    std::string name_;
    std::string image_;
    int width_;
    int height_;
    std::map<std::string, SpriteRegion> regions_;
    std::vector<std::string> sprites_;
};

};  // namespace v3d::config
