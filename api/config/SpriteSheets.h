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
