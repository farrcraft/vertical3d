/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include "SpriteSheet.h"

#include <map>
#include <string>
#include <vector>

#include <boost/json/object.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::config {

/**
 * Named sprite sheets, loaded from a document the config names as Type::Sprite.
 *
 * The same shape as CameraProfiles: one document holds every sheet an app has, each named,
 * and this hands them out by name. What a sprite *means* (whether it animates, what it
 * stands on, how big it is drawn) is up to the app, as with a camera profile.
 *
 * **Unlike the other config readers this one also writes**, because a tool produces a sprite
 * sheet rather than a person. A packer writes the document through document() rather than
 * its own code, since a second implementation could drift silently. get() returns an empty
 * region for a missing name and uv() returns false, so a badly written sheet draws nothing
 * and reports nothing. load() and document() read and write the same table.
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
     * A sheet with no name, no image or no size is rejected and the rest are kept, so one
     * bad entry does not cost an app every sprite it has. CameraProfiles::load differs: it
     * stops at the first bad profile. A region outside its sheet is skipped the same way,
     * because SpriteSheet::place() refuses it.
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
     * Sheets come out in the order they went in and so do the sprites within them, so
     * re-packing a sheet moves only what actually moved and the diff is one a person can
     * read. asset::writeDocument() writes it to a file; this decides only what the document
     * holds.
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
