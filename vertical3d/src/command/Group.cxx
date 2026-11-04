/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Group.h"

#include <api/type/Transform.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/item/Group.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Scene.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor::command {

/**
 **/
Group::Group(const boost::shared_ptr<scene::Scene>& scene, const scene::Items& items) :
    scene_(scene),
    group_(boost::make_shared<scene::item::Group>()) {
    group_->name("Group");
    if (!scene_) {
        return;
    }
    scene::Items gathered;
    for (const boost::shared_ptr<scene::item::Item>& item : scene_->uppermost(items)) {
        if (scene_->item(item->id()) == item) {
            gathered.push_back(item);
        }
    }
    if (gathered.empty()) {
        return;
    }

    const boost::shared_ptr<scene::item::Item> first = scene_->parent(gathered.front()->id());
    const bool shared = std::all_of(gathered.begin(), gathered.end(),
        [&](const boost::shared_ptr<scene::item::Item>& item) { return scene_->parent(item->id()) == first; });
    if (shared) {
        parent_ = first;
    }

    // the centre is in the space of the group's parent. An item already beneath that parent
    // gives its translation, and any other item, which then has no parent, its world origin.
    // Both are where the item is shown, which a curve may decide, and an undo gives the item
    // back its own placement
    const scene::Evaluation evaluation = scene_->evaluate();
    glm::vec3 centre(0.0F);
    std::vector<v3d::type::Transform> shown;
    for (const boost::shared_ptr<scene::item::Item>& item : gathered) {
        const Member member{ .item = item, .parent = scene_->parent(item->id()), .before = item->placement() };
        shown.push_back(evaluation.pose(*item).placement);
        centre += member.parent == parent_ ? shown.back().position : glm::vec3(evaluation.placed(item->id()).world[3]);
        members_.push_back(member);
    }
    centre /= static_cast<float>(members_.size());
    group_->translation(centre);

    for (std::size_t index = 0; index < members_.size(); index++) {
        Member& member = members_[index];
        member.after = shown[index];
        if (member.parent == parent_) {
            // the group only moves the origin, so the rest of the placement carries over exactly
            member.after.position -= centre;
            continue;
        }
        // the world matrix less the group's offset, taken apart. Any shear it holds is lost
        member.after = scene::placement(glm::translate(glm::mat4(1.0F), -centre) * evaluation.placed(member.item->id()).world);
    }
}

/**
 **/
void Group::redo() {
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
void Group::undo() {
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
std::string Group::name() const {
    return "group";
}

/**
 **/
const boost::shared_ptr<scene::item::Group>& Group::group() const noexcept {
    return group_;
}

/**
 **/
bool Group::empty() const noexcept {
    return members_.empty();
}

};  // namespace v3d::editor::command
