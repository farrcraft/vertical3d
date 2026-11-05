/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Units.h>

#include <string>

#include "Command.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 * A change to the unit of length the document measures in, which the document saves, so it is
 * undone like any other change to the document. Nothing in the scene moves.
 **/
class Units final : public Command {
 public:
    /**
     * @param unit the unit the document measures in once the change is done
     **/
    Units(const boost::shared_ptr<scene::Scene>& scene, scene::Length unit);

    void undo() override;
    void redo() override;
    std::string name() const override;

 private:
    boost::shared_ptr<scene::Scene> scene_;
    scene::Length before_;
    scene::Length after_;
};

};  // namespace v3d::editor::command
