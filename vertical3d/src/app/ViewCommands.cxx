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
#include <vertical3d/src/control/AreaChrome.h>
#include <vertical3d/src/view/Shading.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 **/
ViewCommands::ViewCommands(const Context& context) :
    context_(context) {
}

/**
 **/
void ViewCommands::registerCommands(CommandTable* table) const {
    display(table, "view::shade::flat", [](std::size_t, ViewPort* view) { view->display().shading = Shading::Flat; });
    display(table, "view::shade::smooth", [](std::size_t, ViewPort* view) { view->display().shading = Shading::Smooth; });
    for (const auto& [name, cull] : std::vector<std::pair<const char*, ViewPort::Cull>>{ { "none", ViewPort::Cull::None },
        { "front", ViewPort::Cull::Front }, { "back", ViewPort::Cull::Back }, { "both", ViewPort::Cull::Both } }) {
        display(table, std::string("view::cull::") + name, [cull](std::size_t, ViewPort* view) { view->display().cull = cull; });
    }
    for (const auto& [name, fill] : std::vector<std::pair<const char*, ViewPort::Fill>>{ { "points", ViewPort::Fill::Points },
        { "lines", ViewPort::Fill::Lines }, { "fill", ViewPort::Fill::Fill } }) {
        display(table, std::string("view::rendermode::front::") + name, [fill](std::size_t, ViewPort* view) { view->display().front = fill; });
        display(table, std::string("view::rendermode::back::") + name, [fill](std::size_t, ViewPort* view) { view->display().back = fill; });
    }
    for (const auto& [name, filter] : std::vector<std::pair<const char*, ViewPort::VisibleFilter>>{
        { "grid", ViewPort::SHOW_GRID }, { "mesh", ViewPort::SHOW_MESH }, { "camera", ViewPort::SHOW_CAMERA },
        { "light", ViewPort::SHOW_LIGHT }, { "handle", ViewPort::SHOW_HANDLE } }) {
        display(table, std::string("view::show::") + name, [filter](std::size_t, ViewPort* view) { view->show(filter, !view->shows(filter)); });
    }
    for (const AreaChrome::FixedView& fixed : AreaChrome::fixedViews()) {
        const std::string profile = fixed.profile;
        display(table, fixed.command, [this, profile](std::size_t area, ViewPort* view) { look(area, view, profile); });
    }
    display(table, "view::camera::orthographic", [this](std::size_t, ViewPort* view) { toggleProjection(view); });
    // view::show::statistics is not a view flag: the readout covers the window rather than one
    // pane, so the shell shows it and nothing here reads it back
}

/**
 **/
void ViewCommands::display(CommandTable* table, const std::string& name,
    const std::function<void(std::size_t area, ViewPort* view)>& change) const {
    table->hold(name, [this, change](const v3d::event::Event& event) {
        if (event.state() == v3d::event::State::Released || event.repeat()) {
            return;
        }
        const std::size_t area = context_.areaOf(event);
        if (const boost::shared_ptr<ViewPort> view = context_.views ? context_.views->of(area) : nullptr) {
            change(area, view.get());
        }
    });
}

/**
 **/
void ViewCommands::look(std::size_t area, ViewPort* view, const std::string& profile) const {
    if (!context_.profiles->has(profile)) {
        context_.logger->get()->error("There is no camera profile called {}", profile);
        return;
    }
    view->look(context_.profiles->get(profile));
    // the editor's state names the camera, so its tab and a saved layout show the new one
    context_.layout->set(context_.layout->areas()[area].editor().id, "camera", boost::json::string(profile));
    context_.logger->get()->info("{} shows the {} camera", view->name(), profile);
}

/**
 **/
void ViewCommands::toggleProjection(ViewPort* view) const {
    const boost::shared_ptr<v3d::type::camera::Camera> camera = view->camera();
    camera->orthographic(!camera->orthographic());
    context_.logger->get()->info("{} is {}", view->name(), camera->orthographic() ? "orthographic" : "perspective");
}

};  // namespace v3d::editor
