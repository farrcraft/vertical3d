/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <string>
#include <vector>

#include "Scene.h"
#include "SceneStore.h"

#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * The project being edited: a directory holding a manifest, its scenes, and the directories
 * the rest of a project's files go in.
 *
 * The manifest, project.json, is a JSON document written atomically. It holds the format
 * version, the project's name and the paths of its scenes, each relative to the directory.
 * The editor edits the first scene. A save makes every directory of the layout that is
 * missing, writes the scene, and then the manifest.
 *
 * Reading replaces everything the scene holds. The history describes a scene that is being
 * thrown away, so the caller clears it.
 *
 * **Autosave.** autosave() writes the scene's changes into a recovery database in the
 * autosave directory, under the scene's own file name, and never into the scene's document.
 * It has a store of its own, so it writes only what changed since the last autosave. A save
 * removes the recovery database, and so does discardRecovery(). One that is newer than the
 * scene's document, or that has no document beside it, holds changes no save kept, and
 * recoverable() says so.
 **/
class Project final {
 public:
    /**
     * The manifest's format version. read() migrates an older manifest forward one version at
     * a time, and refuses a later one.
     **/
    static const int VERSION;

    /**
     * The manifest's name within the project directory.
     **/
    static const char* const MANIFEST;

    /**
     * The directories every project has, relative to the project directory.
     **/
    static const std::vector<std::string>& layout();

    /**
     * @param directory the project a save writes to until one is opened or saved elsewhere
     **/
    Project(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::filesystem::path& directory);

    /**
     * Open a project by its manifest, and read its first scene into the scene.
     *
     * Nothing changes, the scene and this project included, unless the manifest and the scene
     * are both read whole.
     **/
    bool open(const boost::filesystem::path& manifest, const boost::shared_ptr<Scene>& scene);

    /**
     * Write the scene into this project's directory.
     **/
    bool save(const boost::shared_ptr<Scene>& scene);

    /**
     * Write the scene as a new project, which becomes this project if the write succeeds.
     *
     * @param directory where the new project goes. It must not exist yet
     **/
    bool saveAs(const boost::filesystem::path& directory, const boost::shared_ptr<Scene>& scene);

    /**
     * Write the scene's changes into the recovery database.
     **/
    bool autosave(const boost::shared_ptr<Scene>& scene);

    /**
     * @return where the recovery database for the scene is
     **/
    boost::filesystem::path recovery() const;

    /**
     * @return whether there is a recovery database newer than the scene's document, or one
     *         with no document beside it
     **/
    bool recoverable() const;

    /**
     * Replace the scene with what the recovery database holds. The database stays until the
     * next save or discardRecovery(), so recovering again gives the same scene.
     **/
    bool recover(const boost::shared_ptr<Scene>& scene);

    /**
     * Remove the recovery database, as a save does, or as discarding the changes does.
     **/
    void discardRecovery();

    /**
     * @return what the last autosave wrote
     **/
    const SceneStore::Written& autosaved() const noexcept;

    /**
     * @return the project's directory
     **/
    const boost::filesystem::path& directory() const noexcept;

    /**
     * @return the scene the editor edits, inside the directory
     **/
    boost::filesystem::path scene() const;

    /**
     * @return what the last save wrote of the scene
     **/
    const SceneStore::Written& written() const noexcept;

    /**
     * @return what the project is called, which the manifest holds
     **/
    const std::string& name() const noexcept;

    /**
     **/
    void name(const std::string& title);

 private:
    /**
     * Write the layout, the scene and the manifest into a directory.
     **/
    bool write(const boost::filesystem::path& directory, const boost::shared_ptr<Scene>& scene);

    boost::shared_ptr<v3d::log::Logger> logger_;
    SceneStore store_;
    SceneStore recoveryStore_;
    boost::filesystem::path directory_;
    std::string name_;
    std::vector<std::string> scenes_;
};

};  // namespace v3d::editor
