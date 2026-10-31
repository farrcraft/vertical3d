/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RenderCommands.h"

#include <api/type/camera/Profile.h>
#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/scene/Scene.h>

namespace v3d::editor {

/**
 **/
RenderCommands::RenderCommands(const Context& context) :
    context_(context) {
}

/**
 **/
void RenderCommands::registerCommands(CommandTable* table) const {
    const Context& context = context_;
    table->press("render::view", [&context]() {
        context.atView("a render is seen", [&context](const v3d::type::camera::Profile&) {
            context.renderWindow->show(*context.document->scene(), context.shot());
        });
    });
    table->press("render::stop", [&context]() { context.renderWindow->stop(); });
    table->press("render::close", [&context]() { context.renderWindow->close(); });
    table->press("render::settings", [&context]() { context.renderWindow->openSettings(context.document->scene()->renderSettings()); });
    table->press("render::settings::apply", [&context]() { context.report(context.renderWindow->applySettings(context.document.get())); });
    table->press("render::settings::cancel", [&context]() { context.renderWindow->closeSettings(); });
    table->press("render::batch", [&context]() {
        context.atView("a batch with no scene camera is seen", [&context](const v3d::type::camera::Profile& view) {
            context.report(context.renderWindow->batch(context.document->scene().get(), view, context.document->project()->directory() / "renders"));
        });
    });
}

};  // namespace v3d::editor
