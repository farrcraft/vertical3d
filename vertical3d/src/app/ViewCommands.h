/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>
#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>

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
    void display(CommandTable* table, const std::string& name, const std::function<void(const Context::Placed& at, view::ViewPort* view)>& change) const;

    /**
     * Show a camera profile in an area's view, keeping the view's region.
     *
     * @param at the area, and the layout of the window it is in
     * @param view the view the area shows
     **/
    void look(const Context::Placed& at, view::ViewPort* view, const std::string& profile) const;

    /**
     * Switch a view between perspective and orthographic.
     **/
    void toggleProjection(view::ViewPort* view) const;

    /**
     * Have an area's view look through a scene camera, from the next frame on, and keep it in the
     * editor's state.
     **/
    void lookThrough(const Context::Placed& at, view::ViewPort* view, std::int64_t camera) const;

    /**
     * Lock or unlock a view's camera to the view: while it is locked, navigating the view moves
     * the scene camera it looks through.
     **/
    static void toggleLock(const Context::Placed& at, view::ViewPort* view);

    /**
     * @return the scene camera and the area a command to look through a camera names, from its
     *         data: the camera's document id, then "@" and the area's id for an area's own menu.
     *         Nothing when the data names no camera
     **/
    static std::optional<std::pair<std::int64_t, std::optional<int>>> named(const v3d::event::Event& event);

    const Context& context_;
};

};  // namespace v3d::editor::app
