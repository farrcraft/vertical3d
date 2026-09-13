/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/kind/Json.h>
#include <api/log/Logger.h>

#include <map>
#include <string>
#include <vector>

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
    SpriteSheet();

    /**
     * Build a sheet to put sprites in, for a packer emitting a document rather than a
     * reader filling one out.
     *
     * The size is given here and not per sprite because it is what place() measures a
     * region against - so a sheet built this way refuses exactly what a sheet read from a
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

    /**
     * Put one sprite in the sheet, which is where a region's own validity is decided: a
     * region running off the sheet would give a uv outside 0..1, and that samples whatever
     * the wrap mode decides rather than reporting anything.
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

/**
 * Named sprite sheets, loaded from a document the config names as Type::Sprite.
 *
 * The same shape as CameraProfiles: one document holds every sheet an app has, each named,
 * and this hands them out by name. What a sprite *means* - whether it animates, what it
 * stands on, how big it is drawn - is the app's, exactly as what a camera profile means is.
 *
 * **Unlike the other config readers this one also writes**, because a sprite sheet is the
 * only one of these documents a tool produces rather than a person: it is packed, and a
 * packer that emits the format from its own code is a second implementation of it that
 * drifts from this one silently - a sheet that stopped being emitted correctly draws as
 * nothing and says nothing, since get() answers a missing name with an empty region and
 * uv() answers false. load() and document() are the same table read and written, so there
 * is nothing for the two halves to disagree about.
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
    bool load(const boost::shared_ptr<v3d::asset::kind::Json>& config);

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

    /**
     * Put a sheet in, replacing any sheet of the same name.
     *
     * **Replacing is what packing one sheet of several means**, and it is why there is no
     * merge here: a tool that wants to keep the sheets it did not pack load()s the document
     * first and writes back what it then holds, and one that does not, does not. The
     * decision stays where the tool's other decisions are rather than being a mode on this.
     *
     * @return whether the sheet is one a document can hold - it needs a name, an image and
     *         a size, which is what load() requires of one it reads
     **/
    bool add(const SpriteSheet& sheet);

    /**
     * Every sheet held, as the document load() reads.
     *
     * Sheets come out in the order they went in and so do the sprites within them, so
     * re-packing a sheet moves only what actually moved and the diff is one a person can
     * read. Writing it to a file is asset::writeDocument's, per ADR-0041: what a document
     * holds is decided here and how it reaches the disk is not.
     *
     * A caller is free to add keys of its own to what comes back - a note saying which tool
     * generated the file, say. load() ignores what it does not recognise.
     **/
    boost::json::value document() const;

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::map<std::string, SpriteSheet> sheets_;
    std::vector<std::string> names_;
};

};  // namespace v3d::config
