/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include "SpriteSheet.h"

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <boost/json/object.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::asset::media {

/**
 * Named sprite sheets, loaded from a sprite sheet document.
 *
 * One document holds every sheet an app has, each named, and get() returns one by name. What
 * a sprite *means*, such as whether it animates or how big it is drawn, is up to the app.
 *
 * **SpriteSheets also writes the document**, because a tool produces a sprite sheet. A packer
 * writes through document() and does not serialise the sheets itself. load() and document()
 * read and write the same format. A badly written sheet reports no error when drawn:
 * SpriteSheet::get() returns an empty region for a missing name, and uv() returns false.
 **/
class SpriteSheets final {
 public:
    /**
     * The name config.json gives the kind of a sprite sheet document.
     **/
    static constexpr std::string_view DOCUMENT = "sprite";

    /**
     * @param logger
     **/
    explicit SpriteSheets(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read every sheet in the document.
     *
     * A sheet with no name, no image or no size is skipped, and the rest are kept. A region
     * outside its sheet is skipped the same way, because SpriteSheet::place() refuses it.
     * CameraProfiles::load() instead stops at the first bad profile.
     *
     * A sheet replaces any sheet of the same name already held, as add() does, and keeps that
     * name's place in names(). A name the document gives more than once logs a warning, and
     * the last sheet of that name is kept.
     *
     * @param doc the parsed sprites document
     * @return whether every sheet in it was understood
     **/
    bool load(const boost::json::object& doc);

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
     * There is no merge. A tool that packs one sheet of several and keeps the others
     * load()s the document first, and writes back everything it then holds.
     *
     * @return whether the sheet is one a document can hold - it needs a name, an image and
     *         a size, as load() requires of one it reads
     **/
    bool add(const SpriteSheet& sheet);

    /**
     * Every sheet held, as the document load() reads.
     *
     * Sheets come out in the order they went in, and so do the sprites in each. Re-packing
     * one sheet therefore changes only that sheet's lines in a diff. asset::writeDocument()
     * writes the result to a file.
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

};  // namespace v3d::asset::media
