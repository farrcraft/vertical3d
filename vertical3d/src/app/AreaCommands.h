/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Commands.h>
#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>

#include <string>

namespace v3d::editor {

/**
 * The commands that change the areas: split, join, swap, maximise, the tabs and the kind of
 * editor an area shows. A header's command names its area by id, and one sent by a key acts on
 * the active area. Each one gives the views their regions again after it runs.
 **/
class AreaCommands final {
 public:
    explicit AreaCommands(const Context& context);

    /**
     * Register every command.
     **/
    void registerCommands(CommandTable* table) const;

 private:
    /**
     * Register a command that changes the areas, followed by what a change of the areas needs.
     **/
    void change(CommandTable* table, const std::string& name, const v3d::event::Commands::Handler& handler) const;

    /**
     * The commands that split, join and swap an area across each of its edges.
     **/
    void registerEdges(CommandTable* table) const;

    /**
     * The commands that open, close and choose an area's tabs, and replace what it shows.
     **/
    void registerTabs(CommandTable* table) const;

    const Context& context_;
};

};  // namespace v3d::editor
