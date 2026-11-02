/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>

namespace v3d::editor::app {

/**
 * The Render menu's commands: a render of the shot, which shows a render view, a stop, a batch
 * of every frame, and the render settings, which shows their editor.
 **/
class RenderCommands final {
 public:
    explicit RenderCommands(const Context& context);

    /**
     * Register every command.
     **/
    void registerCommands(CommandTable* table) const;

 private:
    const Context& context_;
};

};  // namespace v3d::editor::app
