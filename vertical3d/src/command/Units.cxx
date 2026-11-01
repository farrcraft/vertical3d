/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Units.h"

#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Units.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 **/
Units::Units(const boost::shared_ptr<scene::Scene>& scene, scene::Length unit) :
    scene_(scene),
    before_(scene ? scene->length() : scene::Length::Metre),
    after_(unit) {
}

/**
 **/
void Units::undo() {
    if (scene_) {
        scene_->length(before_);
    }
}

/**
 **/
void Units::redo() {
    if (scene_) {
        scene_->length(after_);
    }
}

/**
 **/
std::string Units::name() const {
    return "unit of length";
}

};  // namespace v3d::editor::command
