/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/Transform.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Scene.h>

#include <string>

#include "Command.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 * An item moved beneath another item, or out from under its parent.
 *
 * The item's placement is changed so that it stays where it was in the world, and its
 * children move with it. That is exact unless the item's world matrix holds a shear, which a
 * placement cannot hold and so is lost.
 *
 * Everything is worked out when the command is made, from the scene as it stands then, and
 * redo() and undo() only apply it.
 **/
class Parent final : public Command {
 public:
    /**
     * What is moved, and where.
     **/
    struct Move final {
        boost::shared_ptr<scene::item::Item> item;
        boost::shared_ptr<scene::item::Item> parent;  /**< what the item goes beneath, or empty to leave it with no parent **/
    };

    Parent(const boost::shared_ptr<scene::Scene>& scene, const Move& move);

    void undo() override;
    void redo() override;
    std::string name() const override;

    /**
     * @return whether the command changes nothing: the item or the parent is not in the scene,
     *         the item already has that parent, or the parent is the item or beneath it
     **/
    bool empty() const noexcept;

 private:
    boost::shared_ptr<scene::Scene> scene_;
    boost::shared_ptr<scene::item::Item> item_;
    // the item's parent before and after, each empty for none
    boost::shared_ptr<scene::item::Item> before_;
    boost::shared_ptr<scene::item::Item> after_;
    v3d::type::Transform placedBefore_;
    v3d::type::Transform placedAfter_;
    bool empty_ = true;
};

};  // namespace v3d::editor::command
