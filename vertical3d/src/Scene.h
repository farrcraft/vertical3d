/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <vector>

#include "SceneVisitor.h"

#include "../../api/brep/BRep.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

    /**
     * What the editor is editing: the meshes of one document.
     *
     * It holds no cameras. The views own theirs and CameraProfiles owns the table they are
     * built from, so a scene that also kept profiles would be a second place to look for
     * the same thing.
     *
     * Meshes are found by the id their dag::Node base carries rather than by position, so a
     * selection outlives an insertion or a removal.
     **/
    class Scene final {
     public:
        /**
         * @return the id of the mesh, which is what a selection names
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
         * @return the selected mesh, or an empty pointer.
         *
         * One thing is selected at a time, so the first selected mesh is the selection.
         * The transform tools act on it, and component selection lives inside it.
         **/
        boost::shared_ptr<v3d::brep::BRep> selection() const;

        /**
         * @return how many meshes the scene holds
         **/
        std::size_t count() const noexcept;

        /**
         * Drop everything, which is what closing a document does.
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
         * Clear the component selection of every mesh, leaving the objects selected - what
         * a select mask change does, since the components of one kind mean nothing to an
         * operation working in another.
         **/
        void deselectComponents() noexcept;

     private:
        std::vector<boost::shared_ptr<v3d::brep::BRep>> meshes_;
    };

};  // namespace v3d::editor
