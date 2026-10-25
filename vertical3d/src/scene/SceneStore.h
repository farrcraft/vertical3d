/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

#include "Item.h"
#include "Scene.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace v3d::editor {

/**
 * A scene document on disk: a SQLite database that holds one scene.
 *
 * Every item is a row of typed columns for what all items share, with a JSON payload for what
 * its kind adds. What all items share includes the item's parent, as a reference to the
 * parent's row, which is empty for an item with no parent. A mesh's topology is three blobs of
 * little-endian arrays in a table of its own. A payload holds every field its kind's version
 * defines and nothing else, so a change to what a kind stores is a new version of that kind.
 *
 * The database's header carries APPLICATION_ID, so a database another program wrote is
 * refused, and SCHEMA_VERSION, the version of its tables. Each kind of item has a version of
 * its own as well, stored in its row. A read refuses any of them that is newer than this build.
 *
 * **A save writes only what changed.** The store remembers each item it last wrote or read,
 * with its revision, its place in the order and its parent, for the one document it last wrote
 * or read. A write to that document writes the rows of the items that are new or whose revision
 * has moved. It moves the rows of items whose place or parent alone changed, and deletes the
 * rows of items the scene no longer holds. A write to any other path, or to that path once the
 * store has failed to write it, writes the whole scene.
 **/
class SceneStore final {
 public:
    /**
     * "V3DS" in the database header's application id.
     **/
    static constexpr std::int32_t APPLICATION_ID = 0x56334453;

    /**
     * The version of the tables. An older one is migrated forward one version at a time,
     * by the first write to it. A read takes it as it is.
     *
     * Version 1 has no parent column, so every item it holds has no parent.
     **/
    static constexpr std::int32_t SCHEMA_VERSION = 2;

    /**
     * The version of each kind's payload and blobs, which a row stores beside them.
     **/
    static constexpr std::int64_t MESH_VERSION = 1;
    static constexpr std::int64_t CAMERA_VERSION = 1;
    static constexpr std::int64_t LIGHT_VERSION = 1;
    static constexpr std::int64_t GROUP_VERSION = 1;

    /**
     * What the last write did.
     **/
    struct Written {
        /** Whether it wrote the whole scene rather than its changes. **/
        bool whole = false;
        /** How many items' rows it wrote. **/
        std::size_t rows = 0;
        /** How many rows it gave a new place in the order or a new parent, and nothing else. **/
        std::size_t moved = 0;
        /** How many rows it deleted. **/
        std::size_t removed = 0;
    };

    explicit SceneStore(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Replace the scene with what a document holds.
     *
     * The scene is left untouched unless the whole document is understood, so a failed open
     * does not also lose the scene in memory. Items come back in their saved order, with their
     * document ids and their parents, and with nothing selected. A document whose parents do
     * not form a set of trees is not understood.
     *
     * @return whether the whole document was read
     **/
    bool read(const std::string& path, Scene* scene);

    /**
     * Write the scene into a document, making the document when there is none. Only what
     * changed is written to the document this store last wrote or read, and the whole scene to
     * any other.
     *
     * Everything is written in one transaction, so a write that fails leaves the document as
     * it was. A document this build could not read is refused rather than overwritten.
     *
     * @return whether the scene was written
     **/
    bool write(const std::string& path, const Scene& scene);

    /**
     * @return what the last write that succeeded did
     **/
    const Written& written() const noexcept;

    /**
     * An item as the document holds it: the item, its revision, its place in the order, and
     * its parent's document id, which is 0 for none.
     **/
    struct Saved {
        boost::weak_ptr<Item> item;
        std::uint64_t revision = 0;
        std::int64_t position = 0;
        std::int64_t parent = 0;
    };

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    // the document this store last wrote or read, and its items by document id
    std::string path_;
    std::map<std::int64_t, Saved> saved_;
    Written written_;
};

};  // namespace v3d::editor
