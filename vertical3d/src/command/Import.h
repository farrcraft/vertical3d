/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/scene/Import.h>
#include <vertical3d/src/scene/Scene.h>

#include <string>

#include "Command.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 * A file's items put into the scene, with their hierarchy and their curves, as one change.
 *
 * Removing an item takes its links and its curves with it, so the command holds both and puts
 * them back each time it is redone. The items at the top of the import become the selection.
 **/
class Import final : public Command {
 public:
    /**
     * @param file what the command is called, which is the name of the file read
     **/
    Import(const boost::shared_ptr<scene::Scene>& scene, const scene::Import& imported, const std::string& file);

    void undo() override;
    void redo() override;
    std::string name() const override;

 private:
    boost::shared_ptr<scene::Scene> scene_;
    scene::Import imported_;
    std::string file_;
};

};  // namespace v3d::editor::command
