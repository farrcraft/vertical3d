/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Settings.h"

#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 **/
Settings::Settings(const boost::shared_ptr<scene::Scene>& scene, const scene::RenderSettings& settings) :
    scene_(scene),
    before_(scene ? scene->renderSettings() : scene::RenderSettings()),
    after_(settings) {
}

/**
 **/
void Settings::undo() {
    if (scene_) {
        scene_->renderSettings(before_);
    }
}

/**
 **/
void Settings::redo() {
    if (scene_) {
        scene_->renderSettings(after_);
    }
}

/**
 **/
std::string Settings::name() const {
    return "render settings";
}

};  // namespace v3d::editor::command
