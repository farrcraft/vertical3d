/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ViewCommands.h"

#include <api/event/Event.h>
#include <api/event/State.h>
#include <api/type/camera/Camera.h>
#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/control/chrome/Area.h>
#include <vertical3d/src/view/Shading.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor::app {

/**
 **/
ViewCommands::ViewCommands(const Context& context) :
    context_(context) {
}

/**
 **/
void ViewCommands::registerCommands(CommandTable* table) const {
    display(table, "view::shade::flat", [](const Context::Placed&, view::ViewPort* view) { view->display().shading = view::Shading::Flat; });
    display(table, "view::shade::smooth", [](const Context::Placed&, view::ViewPort* view) { view->display().shading = view::Shading::Smooth; });
    for (const auto& [name, cull] : std::vector<std::pair<const char*, view::ViewPort::Cull>>{ { "none", view::ViewPort::Cull::None },
        { "front", view::ViewPort::Cull::Front }, { "back", view::ViewPort::Cull::Back }, { "both", view::ViewPort::Cull::Both } }) {
        display(table, std::string("view::cull::") + name, [cull](const Context::Placed&, view::ViewPort* view) { view->display().cull = cull; });
    }
    for (const auto& [name, fill] : std::vector<std::pair<const char*, view::ViewPort::Fill>>{ { "points", view::ViewPort::Fill::Points },
        { "lines", view::ViewPort::Fill::Lines }, { "fill", view::ViewPort::Fill::Fill } }) {
        display(table, std::string("view::rendermode::front::") + name, [fill](const Context::Placed&, view::ViewPort* view) { view->display().front = fill; });
        display(table, std::string("view::rendermode::back::") + name, [fill](const Context::Placed&, view::ViewPort* view) { view->display().back = fill; });
    }
    for (const auto& [name, filter] : std::vector<std::pair<const char*, view::ViewPort::VisibleFilter>>{
        { "grid", view::ViewPort::SHOW_GRID }, { "mesh", view::ViewPort::SHOW_MESH }, { "camera", view::ViewPort::SHOW_CAMERA },
        { "light", view::ViewPort::SHOW_LIGHT }, { "handle", view::ViewPort::SHOW_HANDLE } }) {
        display(table, std::string("view::show::") + name, [filter](const Context::Placed&, view::ViewPort* view) { view->show(filter, !view->shows(filter)); });
    }
    for (const control::chrome::Area::FixedView& fixed : control::chrome::Area::fixedViews()) {
        const std::string profile = fixed.profile;
        display(table, fixed.command, [this, profile](const Context::Placed& at, view::ViewPort* view) { look(at, view, profile); });
    }
    display(table, "view::camera::orthographic", [this](const Context::Placed&, view::ViewPort* view) { toggleProjection(view); });
    // view::show::statistics is not a view flag: the readout covers the window rather than one
    // pane, so the shell shows it and nothing here reads it back
}

/**
 **/
void ViewCommands::display(CommandTable* table, const std::string& name,
    const std::function<void(const Context::Placed& at, view::ViewPort* view)>& change) const {
    table->hold(name, [this, change](const v3d::event::Event& event) {
        if (event.state() == v3d::event::State::Released || event.repeat()) {
            return;
        }
        const Context::Placed at = context_.placeOf(event);
        if (const boost::shared_ptr<view::ViewPort> view = at.views ? at.views->of(at.area) : nullptr) {
            change(at, view.get());
        }
    });
}

/**
 **/
void ViewCommands::look(const Context::Placed& at, view::ViewPort* view, const std::string& profile) const {
    if (!context_.profiles->has(profile)) {
        context_.logger->get()->error("There is no camera profile called {}", profile);
        return;
    }
    view->look(context_.profiles->get(profile));
    // the editor's state names the camera, so its tab and a saved layout show the new one
    at.layout->set(at.layout->areas()[at.area].editor().id, "camera", boost::json::string(profile));
    context_.logger->get()->info("{} shows the {} camera", view->name(), profile);
}

/**
 **/
void ViewCommands::toggleProjection(view::ViewPort* view) const {
    const boost::shared_ptr<v3d::type::camera::Camera> camera = view->camera();
    camera->orthographic(!camera->orthographic());
    context_.logger->get()->info("{} is {}", view->name(), camera->orthographic() ? "orthographic" : "perspective");
}

};  // namespace v3d::editor::app
