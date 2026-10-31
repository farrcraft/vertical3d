/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/app/WorkspaceSwitcher.h>

#include <functional>
#include <string>
#include <vector>

namespace v3d::editor::app {

/**
 * The project's files: opening a project, saving it under a new name, importing a scene into
 * it, exporting it as RIB, and the questions asked before unsaved changes are lost or an
 * autosave is recovered. The file chooser and the prompt carry out the asking.
 **/
class ProjectFiles final {
 public:
    /**
     * @param workspaces records the arrangement left behind, and shows a project's own
     **/
    ProjectFiles(const Context& context, WorkspaceSwitcher* workspaces);

    /**
     * Register the project's commands, and the file chooser's and the prompt's.
     **/
    void registerCommands(CommandTable* table) const;

    /**
     * Run something that replaces the document, asking first when it has unsaved changes.
     * Saving and then continuing, or discarding them and continuing, both run it. A save that
     * fails stops it, and so does cancelling.
     **/
    void askBeforeDiscarding(const std::function<void()>& proceed) const;

    /**
     * Offer to recover the project's autosave, when it holds changes no save kept.
     **/
    void offerRecovery() const;

 private:
    /**
     * Choose a project to read over the document, or a name to write it under.
     **/
    void open() const;
    void saveAs() const;

    /**
     * Choose a file to import into the document, of the extensions one format is read from.
     **/
    void importScene(const std::vector<std::string>& extensions) const;

    /**
     * Write the scene as RIB beside the executable, seen from the shot's camera.
     **/
    void exportRib() const;

    const Context& context_;
    WorkspaceSwitcher* workspaces_;
};

};  // namespace v3d::editor::app
