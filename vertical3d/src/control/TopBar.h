/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Interface.h>
#include <api/ui/paint/Text.h>

#include <string>
#include <string_view>
#include <vector>

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * What sits on the menu bar's row beside its menus: a tab for each workspace, the project's name
 * with a mark while it has unsaved changes, and a button that opens the command search.
 *
 * They are a ui container, built as the editor runs and listed after the editor's own so they
 * are drawn over the menu bar's band, which takes no press beside its labels. The tabs follow
 * the menus, and the name and the button sit at the right end. A tab sends workspace::show with
 * the workspace's name, and the one shown is checked.
 *
 * sync() builds the container again when the workspaces change, and otherwise moves it and
 * writes the name.
 **/
class TopBar final {
 public:
    /**
     * The container's name.
     **/
    static constexpr std::string_view NAME = "top-bar";

    /**
     * What the bar shows.
     **/
    struct State final {
        std::vector<std::string> workspaces;
        std::string shown;      /**< the workspace shown **/
        std::string project;    /**< the project's name **/
        bool dirty = false;     /**< whether it has changes that are not saved **/
    };

    /**
     * @param before the container this one goes before
     * @param measure how wide a string is at the size the ui is drawn
     **/
    TopBar(const boost::shared_ptr<v3d::ui::Interface>& ui, const std::string& before, const v3d::ui::paint::Measure& measure);

    /**
     * Build the container again if the workspaces changed, put it in a band of the window, and
     * show the project's name and which workspace is shown.
     *
     * @param band where it goes: from the end of the menus to the right edge of the window, as
     *        tall as the menu bar
     **/
    void sync(const State& state, const glm::vec4& band);

    /**
     * @return the container a set of workspaces is drawn with, as Interface::add() reads it
     **/
    static boost::json::object document(const std::vector<std::string>& workspaces);

    /**
     * @return what the bar names a project: its name, and a mark after it while it has unsaved
     *         changes
     **/
    static std::string title(const std::string& project, bool dirty);

 private:
    boost::shared_ptr<v3d::ui::Interface> ui_;
    std::string before_;
    v3d::ui::paint::Measure measure_;
    // the workspaces the container was last built for
    std::vector<std::string> built_;
    bool added_ = false;
};

};  // namespace v3d::editor
