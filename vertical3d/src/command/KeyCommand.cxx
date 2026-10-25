/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "KeyCommand.h"

#include <api/type/animation/Curve.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Properties.h>
#include <vertical3d/src/scene/Scene.h>

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 **/
KeyCommand::KeyCommand(const boost::shared_ptr<Scene>& scene, const Items& items, float frame) :
    scene_(scene) {
    if (!scene_) {
        return;
    }
    for (const boost::shared_ptr<Item>& item : items) {
        if (!item || scene_->item(item->id()) != item) {
            continue;
        }
        const Curves curves = scene_->curves(item->id());
        // the angles the rotation's curves reach at this frame, or none where there is no curve
        glm::vec3 near(0.0F);
        const char* const axes[3] = { "rotation.x", "rotation.y", "rotation.z" };
        for (int axis = 0; axis < 3; axis++) {
            const auto found = curves.find(axes[axis]);
            if (found != curves.end()) {
                near[axis] = found->second.sample(frame);
            }
        }
        for (const auto& [path, value] : values(*item, near)) {
            const auto found = curves.find(path);
            Change change{ .item = item, .path = path };
            if (found != curves.end()) {
                change.before = found->second;
            }
            change.after = change.before;
            change.after.set(frame, value);
            changes_.push_back(change);
        }
    }
}

/**
 **/
void KeyCommand::redo() {
    apply(true);
}

/**
 **/
void KeyCommand::undo() {
    apply(false);
}

/**
 **/
std::string KeyCommand::name() const {
    return "set key";
}

/**
 **/
bool KeyCommand::empty() const noexcept {
    return changes_.empty();
}

/**
 **/
void KeyCommand::apply(bool after) {
    if (!scene_ || changes_.empty()) {
        return;
    }
    for (const Change& change : changes_) {
        scene_->curve(change.item->id(), change.path, after ? change.after : change.before);
    }
    scene_->pose(static_cast<float>(scene_->timeline().frame));
}

};  // namespace v3d::editor
