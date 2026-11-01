/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Key.h"

#include <api/animation/Curve.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Properties.h>
#include <vertical3d/src/scene/Scene.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor::command {

/**
 **/
Key::Key(const boost::shared_ptr<scene::Scene>& scene, const std::vector<Target>& targets, float frame) :
    scene_(scene) {
    if (!scene_) {
        return;
    }
    const scene::Evaluation evaluation = scene_->evaluate();
    for (const auto& [item, paths] : targets) {
        if (!item || scene_->item(item->id()) != item) {
            continue;
        }
        const scene::Curves curves = scene_->curves(item->id());
        // the angles the rotation's curves reach at this frame, or none where there is no curve
        glm::vec3 near(0.0F);
        const char* const axes[3] = { "rotation.x", "rotation.y", "rotation.z" };
        for (int axis = 0; axis < 3; axis++) {
            const auto found = curves.find(axes[axis]);
            if (found != curves.end()) {
                near[axis] = found->second.sample(frame);
            }
        }
        for (const auto& [path, value] : values(item->kind(), evaluation.pose(*item), near)) {
            if (!paths.contains(path)) {
                continue;
            }
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
void Key::redo() {
    apply(true);
}

/**
 **/
void Key::undo() {
    apply(false);
}

/**
 **/
std::string Key::name() const {
    return "set key";
}

/**
 **/
bool Key::empty() const noexcept {
    return changes_.empty();
}

/**
 **/
std::vector<Key::Target> Key::every(const scene::Items& items) {
    std::vector<Target> targets;
    for (const boost::shared_ptr<scene::Item>& item : items) {
        if (item) {
            const std::vector<std::string>& declared = scene::properties(item->kind());
            targets.push_back({ .item = item, .paths = std::set<std::string>(declared.begin(), declared.end()) });
        }
    }
    return targets;
}

/**
 **/
void Key::apply(bool after) {
    if (!scene_ || changes_.empty()) {
        return;
    }
    for (const Change& change : changes_) {
        scene_->curve(change.item->id(), change.path, after ? change.after : change.before);
    }
}

};  // namespace v3d::editor::command
