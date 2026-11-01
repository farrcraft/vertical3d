/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <string>

#include "Command.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 * A change to the document's render settings, which the document saves, so it is undone like
 * any other change to the document.
 **/
class Settings final : public Command {
 public:
    /**
     * @param settings what the settings become. What they were is read from the scene now
     **/
    Settings(const boost::shared_ptr<scene::Scene>& scene, const scene::RenderSettings& settings);

    void undo() override;
    void redo() override;
    std::string name() const override;

 private:
    boost::shared_ptr<scene::Scene> scene_;
    scene::RenderSettings before_;
    scene::RenderSettings after_;
};

};  // namespace v3d::editor::command
