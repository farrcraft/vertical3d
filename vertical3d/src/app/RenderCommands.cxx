/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RenderCommands.h"

#include <api/type/camera/Profile.h>
#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/control/RenderSettingsEditor.h>
#include <vertical3d/src/control/RenderView.h>

namespace v3d::editor::app {

/**
 **/
RenderCommands::RenderCommands(const Context& context) :
    context_(context) {
}

/**
 **/
void RenderCommands::registerCommands(CommandTable* table) const {
    const Context& context = context_;
    table->script("render::view", [&context]() {
        context.atView("a render is seen", [&context](const v3d::type::camera::Profile&) {
            context.renderView->render(*context.document->scene(), context.shot());
            context.show(control::RenderView::KIND, true);
        });
    });
    table->script("render::stop", [&context]() { context.renderView->stop(); });
    table->script("render::settings", [&context]() { context.show(control::RenderSettingsEditor::KIND); });
    table->script("render::batch", [&context]() {
        context.atView("a batch with no scene camera is seen", [&context](const v3d::type::camera::Profile& view) {
            context.report(context.renderView->batch(context.document->scene().get(), view, context.document->project()->directory() / "renders"));
        });
    });
}

};  // namespace v3d::editor::app
