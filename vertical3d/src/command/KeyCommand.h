/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/animation/Curve.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Scene.h>

#include <string>
#include <vector>

#include "Command.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * A key on every property of each of a set of items, at one frame.
 *
 * Each value is the item's as it stands, so an edit made since the scene was last posed is what
 * the key records. A rotation's Euler angles are the ones closest to its curves' values at that
 * frame, so a curve does not jump by a turn.
 *
 * Everything is worked out when the command is made, from the scene as it stands then. Undo and
 * redo put each curve back as it was or as the key left it, and pose the scene at its current
 * frame again.
 **/
class KeyCommand final : public Command {
 public:
    /**
     * @param items what to key. An item the scene does not hold is skipped
     **/
    KeyCommand(const boost::shared_ptr<Scene>& scene, const Items& items, float frame);

    void undo() override;
    void redo() override;
    std::string name() const override;

    /**
     * @return whether there is anything to key. A command with nothing does nothing
     **/
    bool empty() const noexcept;

 private:
    /** One curve the key changes, before and after. An empty curve is no curve. **/
    struct Change {
        boost::shared_ptr<Item> item;
        std::string path;
        v3d::animation::Curve before;
        v3d::animation::Curve after;
    };

    /**
     * Give every changed property the curve one end of the change has, and pose the scene.
     **/
    void apply(bool after);

    boost::shared_ptr<Scene> scene_;
    std::vector<Change> changes_;
};

};  // namespace v3d::editor
