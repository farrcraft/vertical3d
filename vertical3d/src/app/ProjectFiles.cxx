/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ProjectFiles.h"

#include <api/type/camera/Profile.h>
#include <api/ui/shell/FileChooser.h>
#include <api/ui/shell/Prompt.h>
#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/app/WorkspaceSwitcher.h>
#include <vertical3d/src/store/Project.h>

#include <functional>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>

namespace v3d::editor::app {

/**
 **/
ProjectFiles::ProjectFiles(const Context& context, WorkspaceSwitcher* workspaces) :
    context_(context),
    workspaces_(workspaces) {
}

/**
 **/
void ProjectFiles::registerCommands(CommandTable* table) const {
    const Context& context = context_;
    table->press("project::load", [this]() { open(); });
    table->script("project::save", [&context]() { context.document->save(); });
    table->press("project::saveAs", [this]() { saveAs(); });
    table->press("project::chooser::pick", [&context]() { context.chooser->pick(); });
    table->press("project::chooser::accept", [&context]() { context.chooser->accept(); });
    table->press("project::chooser::cancel", [&context]() { context.chooser->close(); });
    table->press("project::export::rib", [this]() { exportRib(); });
    table->press("project::import::gltf", [this]() { importScene({ ".gltf", ".glb" }); });
    table->press("project::import::usd", [this]() { importScene({ ".usd", ".usda", ".usdc", ".usdz" }); });
    // the prompt's buttons, in the order its answers are given
    table->press("project::prompt::answer0", [&context]() { context.prompt->answer(0); });
    table->press("project::prompt::answer1", [&context]() { context.prompt->answer(1); });
    table->press("project::prompt::answer2", [&context]() { context.prompt->answer(2); });
}

/**
 **/
void ProjectFiles::askBeforeDiscarding(const std::function<void()>& proceed) const {
    const Context& context = context_;
    if (!context.document->dirty() || !context.prompt) {
        proceed();
        return;
    }
    context.prompt->open("Save the changes to " + context.document->project()->directory().filename().string() + "?", {
        { "Save", [&context, proceed]() {
            // a project that could not be written still has changes to lose
            if (context.document->save()) {
                proceed();
            }
        } },
        { "Discard", [&context, proceed]() {
            // the changes are thrown away, so an autosave of them is not offered again
            context.document->discardRecovery();
            proceed();
        } },
        { "Cancel", {} }
    }, {});
}

/**
 **/
void ProjectFiles::offerRecovery() const {
    const Context& context = context_;
    if (!context.document->recoverable() || !context.prompt) {
        return;
    }
    context.prompt->open("Recover the unsaved changes to " + context.document->project()->directory().filename().string() + "?", {
        { "Recover", [&context]() {
            context.transformTool->cancel();
            context.document->recover();
        } },
        { "Discard", [&context]() { context.document->discardRecovery(); } }
    }, {});
}

/**
 **/
void ProjectFiles::open() const {
    askBeforeDiscarding([this]() {
        // a project is opened by its manifest
        context_.chooser->open(v3d::ui::shell::FileChooser::Mode::Open, context_.document->project()->directory().parent_path(), { ".json" },
            [this](const boost::filesystem::path& chosen) {
                // a gesture under way is holding the mesh it started on, which the read is
                // about to take out of the scene
                context_.transformTool->cancel();
                workspaces_->record();
                if (context_.document->open(chosen)) {
                    workspaces_->projectChanged();
                    offerRecovery();
                }
            });
    });
}

/**
 **/
void ProjectFiles::saveAs() const {
    // the name typed is the new project's directory, so no extension is added to it
    context_.chooser->open(v3d::ui::shell::FileChooser::Mode::Save, context_.document->project()->directory().parent_path(), {},
        [this](const boost::filesystem::path& chosen) {
            if (context_.document->saveAs(chosen)) {
                workspaces_->projectChanged();
            }
        });
}

/**
 **/
void ProjectFiles::importScene(const std::vector<std::string>& extensions) const {
    const Context& context = context_;
    context.chooser->open(v3d::ui::shell::FileChooser::Mode::Open, context.document->project()->directory().parent_path(), extensions,
        [&context](const boost::filesystem::path& chosen) { context.report(context.document->importScene(chosen)); });
}

/**
 **/
void ProjectFiles::exportRib() const {
    const Context& context = context_;
    context.atView("a scene is exported", [&context](const v3d::type::camera::Profile&) {
        // the export goes in the project's directory, and the picture is named beside it, so
        // rendering the file writes it there
        const boost::filesystem::path& directory = context.document->project()->directory();
        context.report(context.document->exportRib(directory / "export.rib", context.shot(), (directory / "export.png").generic_string()));
    });
}

};  // namespace v3d::editor::app
