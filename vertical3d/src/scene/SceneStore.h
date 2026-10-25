/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <cstdint>
#include <string>

#include "Scene.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * A scene document on disk: a SQLite database that holds one scene.
 *
 * Every item is a row of typed columns for what all items share, with a JSON payload for what
 * its kind adds. A mesh's topology is three blobs of little-endian arrays in a table of its
 * own. A payload holds every field its kind's version defines and nothing else, so a change to
 * what a kind stores is a new version of that kind.
 *
 * The database's header carries APPLICATION_ID, so a database another program wrote is
 * refused, and SCHEMA_VERSION, the version of its tables. Each kind of item has a version of
 * its own as well, stored in its row. A read refuses any of them that is newer than this build.
 **/
class SceneStore final {
 public:
    /**
     * "V3DS" in the database header's application id.
     **/
    static constexpr std::int32_t APPLICATION_ID = 0x56334453;

    /**
     * The version of the tables. An older one is migrated forward one version at a time.
     * This is the first version, so a lower one is refused.
     **/
    static constexpr std::int32_t SCHEMA_VERSION = 1;

    /**
     * The version of each kind's payload and blobs, which a row stores beside them.
     **/
    static constexpr std::int64_t MESH_VERSION = 1;
    static constexpr std::int64_t CAMERA_VERSION = 1;
    static constexpr std::int64_t LIGHT_VERSION = 1;

    explicit SceneStore(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Replace the scene with what a document holds.
     *
     * The scene is left untouched unless the whole document is understood, so a failed open
     * does not also lose the scene in memory. Items come back in their saved order, with their
     * document ids, and with nothing selected.
     *
     * @return whether the whole document was read
     **/
    bool read(const std::string& path, Scene* scene) const;

    /**
     * Write the scene as the whole of a document, making the document when there is none.
     *
     * Everything is written in one transaction, so a write that fails leaves the document as
     * it was. A document this build could not read is refused rather than overwritten.
     *
     * @return whether the scene was written
     **/
    bool write(const std::string& path, const Scene& scene) const;

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
};

};  // namespace v3d::editor
