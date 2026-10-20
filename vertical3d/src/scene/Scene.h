/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>

#include <cstddef>
#include <vector>

#include "SceneVisitor.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * What the editor is editing: the meshes of one document.
 *
 * It holds no cameras. The views own theirs, and config::CameraProfiles owns the table
 * they are built from.
 *
 * Meshes are found by the id their dag::Node base carries rather than by position, so a
 * selection outlives an insertion or a removal.
 **/
class Scene final {
 public:
    /**
     * @return the id of the mesh. A selection names a mesh by this id.
     **/
    unsigned int add(const boost::shared_ptr<v3d::brep::BRep>& mesh);

    /**
     * @return whether a mesh of that id was there to remove
     **/
    bool remove(unsigned int id);

    /**
     * @return the mesh of that id, or an empty pointer
     **/
    boost::shared_ptr<v3d::brep::BRep> mesh(unsigned int id) const;

    /**
     * @return every selected mesh, in insertion order. Component selection lives inside
     *         these meshes, and may span several of them.
     **/
    std::vector<boost::shared_ptr<v3d::brep::BRep>> selection() const;

    /**
     * @return the first selected mesh in insertion order, or an empty pointer. The
     *         manipulators act on this one mesh.
     **/
    boost::shared_ptr<v3d::brep::BRep> firstSelected() const;

    /**
     * @return how many meshes the scene holds
     **/
    std::size_t count() const noexcept;

    /**
     * Drop everything, as closing a document does.
     **/
    void clear() noexcept;

    /**
     * Walk the meshes in insertion order.
     **/
    void accept(SceneVisitor* visitor) const;

    /**
     * Clear the selection of every mesh and of every component of every mesh.
     **/
    void deselect() noexcept;

    /**
     * Clear the component selection of every mesh, and leave the objects selected. A select
     * mask change does this, because components of one kind mean nothing to an operation
     * working on another.
     **/
    void deselectComponents() noexcept;

 private:
    std::vector<boost::shared_ptr<v3d::brep::BRep>> meshes_;
};

};  // namespace v3d::editor
