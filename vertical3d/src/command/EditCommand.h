/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/scene/Curve.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Properties.h>

#include <string>

#include "Command.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * An edit of one item's own values in the properties editor: its name, its placement, a
 * camera's lens, a light's emission or a curve item's style.
 *
 * The edit is made as it happens, as a drag of a number moves it, so by the time the command is
 * made the change is done. The command records the values at both ends of the edit, and undo
 * and redo write either end back whole.
 **/
class EditCommand final : public Command {
 public:
    /**
     * What an edit can change of an item. A kind that has no lens, emission or style keeps that
     * one at its default, and it is not written.
     **/
    struct Values {
        std::string name;
        Pose pose;
        Curve::Style style;
    };

    /**
     * @return the item's own values
     **/
    static Values of(const Item& item);

    /**
     * Write values into an item: the name and the placement, and the lens, emission or style
     * its kind has.
     **/
    static void apply(Item* item, const Values& values);

    /**
     * @return whether two sets of values are the same, within the tolerance a placement is
     *         compared at
     **/
    static bool same(const Values& first, const Values& second);

    /**
     * @param before the item's values when the edit began. What they became is read from the
     *        item now
     * @param what what to call the change, such as "rename"
     **/
    EditCommand(const boost::shared_ptr<Item>& item, const Values& before, const std::string& what);

    void undo() override;
    void redo() override;
    std::string name() const override;

 private:
    boost::shared_ptr<Item> item_;
    Values before_;
    Values after_;
    std::string what_;
};

};  // namespace v3d::editor
