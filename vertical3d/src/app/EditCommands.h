/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>

namespace v3d::editor::app {

/**
 * The commands that change the document: making items, grouping them, keying them, auto key,
 * the modelling operations and undo. Each one reports what the document said.
 **/
class EditCommands final {
 public:
    explicit EditCommands(const Context& context);

    /**
     * Register every command. A key binding in mappings.json and a menu item in the ui config
     * invoke a command by these names, so all three spell them the same way.
     **/
    void registerCommands(CommandTable* table) const;

 private:
    /**
     * The commands that make an item: the primitives, the cameras, the lights, the curves and
     * a group.
     **/
    void registerCreate(CommandTable* table) const;

    /**
     * The modelling operations, each under the select mask whose parts it works on.
     **/
    void registerModelling(CommandTable* table) const;

    const Context& context_;
};

};  // namespace v3d::editor::app
