/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Parent.h"

#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Scene.h>

#include <algorithm>
#include <string>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>

namespace v3d::editor::command {

/**
 **/
Parent::Parent(const boost::shared_ptr<scene::Scene>& scene, const Move& move) :
    scene_(scene),
    item_(move.item),
    after_(move.parent) {
    if (!scene_ || !item_ || scene_->item(item_->id()) != item_) {
        return;
    }
    if (after_ && scene_->item(after_->id()) != after_) {
        return;
    }
    before_ = scene_->parent(item_->id());
    if (before_ == after_) {
        return;
    }
    if (after_) {
        const scene::Items above = scene_->ancestors(after_->id());
        if (after_ == item_ || std::ranges::find(above, item_) != above.end()) {
            return;
        }
    }
    empty_ = false;
    placedBefore_ = item_->placement();
    const scene::Evaluation evaluation = scene_->evaluate();
    const glm::mat4 under = after_ ? evaluation.placed(after_->id()).world : glm::mat4(1.0F);
    placedAfter_ = scene::placement(glm::inverse(under) * evaluation.placed(item_->id()).world);
}

/**
 **/
void Parent::redo() {
    if (empty_) {
        return;
    }
    scene_->parent(item_->id(), after_ ? after_->id() : 0);
    item_->placement(placedAfter_);
}

/**
 **/
void Parent::undo() {
    if (empty_) {
        return;
    }
    scene_->parent(item_->id(), before_ ? before_->id() : 0);
    item_->placement(placedBefore_);
}

/**
 **/
std::string Parent::name() const {
    return "parent";
}

/**
 **/
bool Parent::empty() const noexcept {
    return empty_;
}

};  // namespace v3d::editor::command
