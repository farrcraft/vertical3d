/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>

#include <string>
#include <vector>

#include "Command.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * An operation that changed which vertices, half edges and faces a mesh holds.
 *
 * It holds the mesh's arrays from before the operation and from after it, and undo and redo
 * put one or the other back whole. The numbering comes back exactly, so a command deeper in
 * the history that names a part by index still names the same part. The operation has already
 * run when the command is made.
 *
 * Selection is not history, and after a renumbering an old flag could name another part, so
 * undo and redo clear the component selection of each mesh they restore. The objects stay
 * selected.
 **/
class TopologyCommand final : public Command {
 public:
    /** One mesh, and its arrays before the operation and after it. **/
    struct Change {
        boost::shared_ptr<v3d::brep::BRep> mesh;
        v3d::brep::BRep::Snapshot before;
        v3d::brep::BRep::Snapshot after;
    };

    /**
     * @param name what the operation is called, which is the name the command reports
     **/
    TopologyCommand(const std::vector<Change>& changes, const std::string& name);

    void undo() override;
    void redo() override;
    std::string name() const override;

 private:
    std::vector<Change> changes_;
    std::string name_;
};

};  // namespace v3d::editor
