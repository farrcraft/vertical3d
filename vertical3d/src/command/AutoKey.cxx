/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "AutoKey.h"

#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 **/
AutoKey::AutoKey(const boost::shared_ptr<scene::Scene>& scene, bool on) :
    scene_(scene),
    on_(on) {
}

/**
 **/
void AutoKey::undo() {
    apply(!on_);
}

/**
 **/
void AutoKey::redo() {
    apply(on_);
}

/**
 **/
std::string AutoKey::name() const {
    return on_ ? "auto key on" : "auto key off";
}

/**
 **/
void AutoKey::apply(bool on) {
    if (!scene_) {
        return;
    }
    scene::Timeline timeline = scene_->timeline();
    timeline.autoKey = on;
    scene_->timeline(timeline);
}

};  // namespace v3d::editor::command
