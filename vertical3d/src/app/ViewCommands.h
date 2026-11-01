/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <functional>
#include <string>

namespace v3d::editor::app {

/**
 * The commands that change how a view draws: its camera, its projection, its shading, fills and
 * cull, and what it shows. Each view keeps its own. A header's command names its area by id, and
 * one from the View menu or a key acts on the active area.
 **/
class ViewCommands final {
 public:
    explicit ViewCommands(const Context& context);

    /**
     * Register every command.
     **/
    void registerCommands(CommandTable* table) const;

 private:
    /**
     * Register a command that changes the view of the area it acts on, on a press.
     **/
    void display(CommandTable* table, const std::string& name, const std::function<void(std::size_t area, view::ViewPort* view)>& change) const;

    /**
     * Show a camera profile in an area's view, keeping the view's region.
     *
     * @param area an index into the layout's areas
     * @param view the view the area shows
     **/
    void look(std::size_t area, view::ViewPort* view, const std::string& profile) const;

    /**
     * Switch a view between perspective and orthographic.
     **/
    void toggleProjection(view::ViewPort* view) const;

    const Context& context_;
};

};  // namespace v3d::editor::app
