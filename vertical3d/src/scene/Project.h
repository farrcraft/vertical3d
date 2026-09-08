/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Scene.h"

#include "../../../api/log/Logger.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * The document: a scene on disk, read and written as JSON per ADR-0018.
 *
 * The topology is stored as it stands rather than as the calls that would rebuild it,
 * so a round trip renumbers nothing. Neither the selection nor a mesh's id is stored -
 * an id comes from a process wide counter and a saved one would collide with a mesh
 * already loaded.
 *
 * Reading replaces everything the scene holds. The history describes a scene that is
 * being thrown away, so the caller clears it.
 **/
class Project final {
 public:
    /**
     * The format version written into every file, and the only one read() accepts.
     **/
    static const int VERSION;

    /**
     **/
    explicit Project(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Replace the scene with what the file holds.
     *
     * The scene is left untouched when the file cannot be read or does not describe a
     * consistent mesh, so a failed open does not also lose the document in memory.
     *
     * @param path the file to read
     * @param scene filled with what it holds
     * @return whether the whole file was understood
     **/
    bool read(const std::string& path, const boost::shared_ptr<Scene>& scene);

    /**
     * Write the scene as a project document.
     *
     * The file already there survives a write that does not complete, per ADR-0041.
     *
     * @param path the file to write, replaced if it exists
     * @param scene what to write
     * @return whether the file was written
     **/
    bool write(const std::string& path, const boost::shared_ptr<Scene>& scene) const;

    /**
     * @return what the document is called, which is read from and written to the file
     **/
    const std::string& name() const noexcept;

    /**
     **/
    void name(const std::string& title);

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::string name_;
};

};  // namespace v3d::editor
