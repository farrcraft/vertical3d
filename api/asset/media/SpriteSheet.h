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

namespace v3d::asset::media {

/**
 * One image and a table of names over rectangles in it.
 *
 * The sheet names its image but does not load it. An app resolves the name through its own
 * asset manager and renderer, as it does the images a theme names.
 *
 * **The rectangles are in pixels and the sheet states its own size.** uv() divides one by the
 * other to give the fractions a shader needs. The pixels are relative to the size the sheet
 * states, not to the image file, so the document stays valid if the image is rescaled.
 **/
class SpriteSheet final {
 public:
    SpriteSheet();

    /**
     * The name, image and size of a sheet a packer builds to write out.
     *
     * place() checks each region against this size, as it does for a sheet read from a
     * document. A packer therefore cannot write a region that SpriteSheets::load() would drop.
     **/
    struct Spec {
        std::string name;
        std::string image;   /**< the image the sprites are cut from **/
        int width = 0;
        int height = 0;
    };

    explicit SpriteSheet(const Spec& spec);

    /**
     * @return the sheet's name
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
     * Put one sprite in the sheet. An empty name, an empty region and a region that runs off
     * the sheet are refused. A region off the sheet would give a uv outside 0..1, and the
     * sampler's wrap mode would then draw something instead of failing visibly.
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

};  // namespace v3d::asset::media
