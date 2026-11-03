/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>

#include <string>

#include <boost/json.hpp>

namespace v3d::editor::app {

/**
 * Which workspace the editor shows, and each project's own arrangement of it. The workspaces
 * themselves are kept by Workspaces. This shows one in the layout, records how the project
 * arranged it, and writes both to the settings.
 **/
class WorkspaceSwitcher final {
 public:
    explicit WorkspaceSwitcher(Context* context);

    /**
     * Register the commands that show, keep, reset, export and import the workspaces. Each one
     * gives the views their regions again after it runs.
     **/
    void registerCommands(CommandTable* table);

    /**
     * Read the workspaces the editor ships with and the ones the settings keep, make the layout,
     * and show the workspace the current project was last shown in, the one last shown, or the
     * first.
     *
     * @param shipped the workspace document the editor ships with, or nothing when it has none
     * @return whether a workspace could be shown
     **/
    bool start(const boost::json::object* shipped);

    /**
     * Show a workspace: the current project's own arrangement of it, or the workspace as it is
     * kept when the project has none.
     *
     * @return whether there is a workspace of that name that could be shown
     **/
    bool show(const std::string& name);

    /**
     * Record the current project's arrangement in the workspace shown, and save the settings.
     **/
    void record();

    /**
     * Show the workspace a project that was just opened or saved under a new name was last shown
     * in, or record the arrangement it has now when it has no record.
     **/
    void projectChanged();

    /**
     * @return the name of the workspace shown
     **/
    const std::string& shown() const noexcept;

 private:
    /**
     * The name of a kept workspace.
     **/
    static std::string nameOf(const boost::json::object& workspace);

    /**
     * @return the workspace shown, as it is arranged: the editor's own window's layout, and under
     *         "windows" each window the editor opened, with its size and layout
     **/
    boost::json::object arranged() const;

    /**
     * Write the workspaces and the projects' records to the settings.
     **/
    void saveSettings();

    /**
     * Show the workspace a step after the one shown, wrapping at either end.
     **/
    void step(int step);

    /**
     * Write the workspace shown, as it is arranged, to a file the user chooses.
     **/
    void exportShown();

    /**
     * Keep the workspaces a file the user chooses holds, and show the first of them.
     **/
    void importChosen();

    Context* context_;
    std::string shown_;
};

};  // namespace v3d::editor::app
