/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>

#include <string>

namespace v3d::editor::app {

/**
 * The commands that choose what the tools do: the select mask, the camera move latched or held,
 * the transform tool's mode, and whether a click adds to the selection.
 **/
class ToolCommands final {
 public:
    explicit ToolCommands(const Context& context);

    /**
     * Register every command.
     **/
    void registerCommands(CommandTable* table) const;

 private:
    /**
     * Choose the transform tool's mode, or the select mask the select tool picks under and the
     * transform tool's handles move.
     **/
    void transformMode(const std::string& name) const;
    void selectMask(const std::string& name) const;

    /**
     * Hold or release a camera move for as long as its modifier is down.
     **/
    void cameraMode(const std::string& name, bool pressed) const;

    const Context& context_;
};

};  // namespace v3d::editor::app
