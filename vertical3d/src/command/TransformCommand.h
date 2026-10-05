/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>

#include <string>

#include "Command.h"
#include "Placement.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * One gesture of a manipulator: where the mesh was when the handle was grabbed, and
 * where it was when the handle was let go.
 *
 * The drag itself has already been applied a motion event at a time - a gesture cannot
 * wait for its own end to show what it is doing - so the command records the two ends
 * of it rather than the offset, per ADR-0016.
 **/
class TransformCommand final : public Command {
 public:
    /**
     * @param mode which manipulator made the change, which is the name it reports
     **/
    TransformCommand(const boost::shared_ptr<v3d::brep::BRep>& mesh,
        const Placement& before,
        const Placement& after,
        const std::string& mode);

    void undo() override;
    void redo() override;
    std::string name() const override;

 private:
    boost::shared_ptr<v3d::brep::BRep> mesh_;
    Placement before_;
    Placement after_;
    std::string mode_;
};

};  // namespace v3d::editor
