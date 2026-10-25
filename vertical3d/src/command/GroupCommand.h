/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Scene.h>

#include <string>
#include <vector>

#include "Command.h"
#include "Placement.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * Items gathered beneath a new group, which becomes the selection.
 *
 * The group is placed at the mean of the items' origins, with no rotation and no scale. It
 * goes beneath the items' parent when they all have the same one, and has no parent
 * otherwise. Each item's placement is changed so that it stays where it was in the world.
 * That is exact for an item that keeps its parent's space. An item moved out from under a
 * sheared parent loses the shear, because a placement cannot hold one.
 *
 * Everything is worked out when the command is made, from the scene as it stands then, and
 * redo() and undo() only apply it.
 **/
class GroupCommand final : public Command {
 public:
    /**
     * @param items what to group. An item beneath another of them is left where it is, and
     *        moves with the other
     **/
    GroupCommand(const boost::shared_ptr<Scene>& scene, const Items& items);

    void undo() override;
    void redo() override;
    std::string name() const override;

    /**
     * @return the group the items go beneath
     **/
    const boost::shared_ptr<Group>& group() const noexcept;

    /**
     * @return whether there is anything to group. A command with nothing does nothing
     **/
    bool empty() const noexcept;

 private:
    /** One item grouped, its parent before, and its placement before and after. **/
    struct Member {
        boost::shared_ptr<Item> item;
        boost::shared_ptr<Item> parent;
        Placement before;
        Placement after;
    };

    boost::shared_ptr<Scene> scene_;
    boost::shared_ptr<Group> group_;
    // the group's own parent, or empty when it has none
    boost::shared_ptr<Item> parent_;
    std::vector<Member> members_;
};

};  // namespace v3d::editor
