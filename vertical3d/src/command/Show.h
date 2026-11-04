/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Scene.h>

#include <string>
#include <vector>

#include "Command.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 * Where items are shown, changed together: hidden, shown, or isolated.
 *
 * An item the change hides from the views is deselected, since it can no longer be picked. That
 * includes an item hidden because an item above it is. Undo selects each again.
 **/
class Show final : public Command {
 public:
    /**
     * One item, and where it is to be shown.
     **/
    struct Change final {
        boost::shared_ptr<scene::item::Item> item;
        scene::item::Item::Shown shown;
    };

    /**
     * Each item's flags before are read when the command is made. A change that leaves an
     * item's flags as they are is dropped.
     *
     * @param name what the change is called, such as "hide"
     **/
    Show(const boost::shared_ptr<scene::Scene>& scene, const std::vector<Change>& changes, const std::string& name);

    void undo() override;
    void redo() override;
    std::string name() const override;

    /**
     * @return whether the command changes nothing
     **/
    bool empty() const noexcept;

 private:
    /**
     * One item's flags before and after.
     **/
    struct Entry final {
        boost::shared_ptr<scene::item::Item> item;
        scene::item::Item::Shown before;
        scene::item::Item::Shown after;
    };

    boost::shared_ptr<scene::Scene> scene_;
    std::vector<Entry> entries_;
    // the selected items the change hid, which undo selects again
    scene::Items deselected_;
    std::string name_;
};

};  // namespace v3d::editor::command
