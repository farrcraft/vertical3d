/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "GroupCommand.h"

#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Scene.h>

#include <algorithm>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 **/
GroupCommand::GroupCommand(const boost::shared_ptr<Scene>& scene, const Items& items) :
    scene_(scene),
    group_(boost::make_shared<Group>()) {
    if (!scene_) {
        return;
    }
    Items gathered;
    for (const boost::shared_ptr<Item>& item : scene_->uppermost(items)) {
        if (scene_->item(item->id()) == item) {
            gathered.push_back(item);
        }
    }
    if (gathered.empty()) {
        return;
    }

    const boost::shared_ptr<Item> first = scene_->parent(gathered.front()->id());
    const bool shared = std::all_of(gathered.begin(), gathered.end(),
        [&](const boost::shared_ptr<Item>& item) { return scene_->parent(item->id()) == first; });
    if (shared) {
        parent_ = first;
    }

    // the centre is in the space of the group's parent. An item already beneath that parent
    // gives its translation, and any other item, which then has no parent, its world origin
    const Evaluation evaluation = scene_->evaluate();
    glm::vec3 centre(0.0F);
    for (const boost::shared_ptr<Item>& item : gathered) {
        const Member member{ .item = item, .parent = scene_->parent(item->id()), .before = item->placement() };
        centre += member.parent == parent_ ? member.before.position : glm::vec3(evaluation.placed(item->id()).world[3]);
        members_.push_back(member);
    }
    centre /= static_cast<float>(members_.size());
    group_->translation(centre);

    for (Member& member : members_) {
        member.after = member.before;
        if (member.parent == parent_) {
            // the group only moves the origin, so the rest of the placement carries over exactly
            member.after.position -= centre;
            continue;
        }
        // taken apart into a rotation and the scale along the item's own axes. Whatever the
        // two cannot hold is a shear, and is lost
        const glm::mat4& world = evaluation.placed(member.item->id()).world;
        member.after.position = glm::vec3(world[3]) - centre;
        member.after.rotation = orientation(world);
        const glm::mat3 stretch = glm::transpose(glm::mat3_cast(member.after.rotation)) * glm::mat3(world);
        member.after.scale = glm::vec3(stretch[0][0], stretch[1][1], stretch[2][2]);
    }
}

/**
 **/
void GroupCommand::redo() {
    if (!scene_ || members_.empty()) {
        return;
    }
    scene_->deselect();
    scene_->add(group_);
    scene_->parent(group_->id(), parent_ ? parent_->id() : 0);
    for (const Member& member : members_) {
        scene_->parent(member.item->id(), group_->id());
        member.item->placement(member.after);
    }
    group_->selected(true);
}

/**
 **/
void GroupCommand::undo() {
    if (!scene_ || members_.empty()) {
        return;
    }
    // the items are moved out before the group goes, since removing it would take them too
    for (const Member& member : members_) {
        scene_->parent(member.item->id(), member.parent ? member.parent->id() : 0);
        member.item->placement(member.before);
        member.item->selected(true);
    }
    group_->selected(false);
    scene_->remove(group_->id());
}

/**
 **/
std::string GroupCommand::name() const {
    return "group";
}

/**
 **/
const boost::shared_ptr<Group>& GroupCommand::group() const noexcept {
    return group_;
}

/**
 **/
bool GroupCommand::empty() const noexcept {
    return members_.empty();
}

};  // namespace v3d::editor
