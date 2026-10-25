/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SettingsCommand.h"

#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 **/
SettingsCommand::SettingsCommand(const boost::shared_ptr<Scene>& scene, const RenderSettings& settings) :
    scene_(scene),
    before_(scene ? scene->renderSettings() : RenderSettings()),
    after_(settings) {
}

/**
 **/
void SettingsCommand::undo() {
    if (scene_) {
        scene_->renderSettings(before_);
    }
}

/**
 **/
void SettingsCommand::redo() {
    if (scene_) {
        scene_->renderSettings(after_);
    }
}

/**
 **/
std::string SettingsCommand::name() const {
    return "render settings";
}

};  // namespace v3d::editor
