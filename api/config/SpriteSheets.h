/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <map>
#include <string>
#include <vector>

#include "../asset/Json.h"
#include "../log/Logger.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::config {

/**
 * Where one sprite sits in the sheet that holds it, in the sheet's own pixels.
 **/
struct SpriteRegion final {
    SpriteRegion() noexcept;

    int x;
    int y;
    int width;
    int height;
};

/**
 * One image and a table of names over rectangles in it.
 *
 * The image is named rather than loaded: an app resolves it through its own asset manager
 * and renderer, per ADR-0020, exactly as it does the images a theme names.
 *
 * **The rectangles are in pixels and the sheet states its own size**, so uv() divides one by
 * the other. An author reads a sprite sheet in pixels and a shader wants a fraction, and
 * converting at load would leave the document holding numbers nobody can check against the
 * image. Stating the size is also what makes the document survive the sheet being rescaled:
 * the pixels are of the size written here, not of whatever the file on disk turns out to be.
 **/
class SpriteSheet final {
 public:
    SpriteSheet() noexcept;

    /**
     * @return what the sheet is called, as whatever names one names it
     **/
    const std::string& name() const noexcept;

    /**
     * @return the image the regions are cut from, for the app to resolve
     **/
    const std::string& image() const noexcept;

    /**
     * @return the size the document says the sheet is, which is what uv() divides by
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

 private:
    friend class SpriteSheets;

    std::string name_;
    std::string image_;
    int width_;
    int height_;
    std::map<std::string, SpriteRegion> regions_;
    std::vector<std::string> sprites_;
};

/**
 * Named sprite sheets, loaded from a document the config names as Type::Sprite.
 *
 * The same shape as CameraProfiles: one document holds every sheet an app has, each named,
 * and this hands them out by name. What a sprite *means* - whether it animates, what it
 * stands on, how big it is drawn - is the app's, exactly as what a camera profile means is.
 **/
class SpriteSheets final {
 public:
    /**
     * @param logger
     **/
    explicit SpriteSheets(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read every sheet in the document.
     *
     * A sheet with no name, no image or no size is rejected and the rest are kept, the way
     * a malformed camera profile is: one bad entry should not cost an app every sprite it
     * has. A region outside the sheet it is in is rejected the same way, because a uv
     * outside 0..1 samples whatever the wrap mode decides rather than reporting anything.
     *
     * @param config the parsed sprites document
     * @return whether every sheet in it was understood
     **/
    bool load(const boost::shared_ptr<v3d::asset::Json>& config);

    /**
     * @param name the sheet name
     * @return the sheet, or an empty one when there is no such sheet
     **/
    SpriteSheet get(const std::string& name) const;

    /**
     * @return whether a sheet of that name was loaded
     **/
    bool has(const std::string& name) const;

    /**
     * @return the sheet names, in the order the document listed them
     **/
    const std::vector<std::string>& names() const noexcept;

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::map<std::string, SpriteSheet> sheets_;
    std::vector<std::string> names_;
};

};  // namespace v3d::config
