/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/animation/Curve.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Scene.h>

#include <set>
#include <string>
#include <vector>

#include "Command.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 * A key on properties of a set of items, at one frame.
 *
 * Each value is the one an evaluation of the current frame shows, so a pending edit is what the
 * key records. A rotation's Euler angles are the ones closest to its curves' values at that
 * frame, so a curve does not jump by a turn.
 *
 * Everything is worked out when the command is made, from the scene as it stands then. Undo and
 * redo put each curve back as it was or as the key left it. A pending edit stays pending, so an
 * undone key shows the edit again until the frame changes.
 **/
class Key final : public Command {
 public:
    /**
     * An item and the properties of it to key. A path its kind does not declare is skipped.
     **/
    struct Target {
        boost::shared_ptr<scene::item::Item> item;
        std::set<std::string> paths;
    };

    /**
     * @return a target for every property of each item
     **/
    static std::vector<Target> every(const scene::Items& items);

    /**
     * @param targets what to key. An item the scene does not hold is skipped
     **/
    Key(const boost::shared_ptr<scene::Scene>& scene, const std::vector<Target>& targets, float frame);

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
        boost::shared_ptr<scene::item::Item> item;
        std::string path;
        v3d::animation::Curve before;
        v3d::animation::Curve after;
    };

    /**
     * Give every changed property the curve one end of the change has.
     **/
    void apply(bool after);

    boost::shared_ptr<scene::Scene> scene_;
    std::vector<Change> changes_;
};

};  // namespace v3d::editor::command
