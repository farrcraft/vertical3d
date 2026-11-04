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

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor::app {

namespace {

/**
 * @return the whole number a piece of text is, or nothing when it is anything else
 **/
std::optional<std::int64_t> whole(std::string_view text) {
    std::int64_t value = 0;
    const std::from_chars_result read = std::from_chars(text.data(), text.data() + text.size(), value);
    if (read.ec != std::errc() || read.ptr != text.data() + text.size() || text.empty()) {
        return std::nullopt;
    }
    return value;
}

};  // namespace

/**
 **/
ViewCommands::ViewCommands(const Context& context) :
    context_(context) {
}

/**
 **/
void ViewCommands::registerCommands(CommandTable* table) const {
    // the shading is kept in the editor's state, so a saved layout and the view's tab show it
    for (const view::Shading shading : { view::Shading::Flat, view::Shading::Smooth }) {
        display(table, "view::shade::" + std::string(view::name(shading)), [shading](const Context::Placed& at, view::ViewPort* view) {
            view->display().shading = shading;
            at.layout->set(at.layout->areas()[at.area].editor().id, "shading", boost::json::string(view::name(shading)));
        });
    }
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
    display(table, "view::camera::lock", [this](const Context::Placed& at, view::ViewPort* view) { toggleLock(at, view); });
    // the camera is named in the data, with the area when an area's own menu sent it
    table->hold("view::camera::scene", [this](const v3d::event::Event& event) {
        if (event.state() == v3d::event::State::Released || event.repeat()) {
            return;
        }
        const std::optional<std::pair<std::int64_t, std::optional<int>>> chosen = named(event);
        if (!chosen) {
            return;
        }
        v3d::event::Event placed(event);
        if (chosen->second) {
            placed.data(*chosen->second);
        }
        const Context::Placed at = context_.placeOf(chosen->second ? placed : v3d::event::Event());
        if (const boost::shared_ptr<view::ViewPort> view = at.views ? at.views->of(at.area) : nullptr) {
            lookThrough(at, view.get(), chosen->first);
        }
    });
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
    view->through({});
    view->look(context_.profiles->get(profile));
    // the editor's state names the camera, so its tab and a saved layout show the new one
    const std::size_t editor = at.layout->areas()[at.area].editor().id;
    at.layout->set(editor, "camera", boost::json::string(profile));
    at.layout->set(editor, "through", 0);
    at.layout->set(editor, "locked", false);
    context_.logger->get()->info("{} shows the {} camera", view->name(), profile);
}

/**
 **/
void ViewCommands::lookThrough(const Context::Placed& at, view::ViewPort* view, std::int64_t camera) const {
    view->through({ .camera = camera, .scene = false, .locked = view->through().locked });
    at.layout->set(at.layout->areas()[at.area].editor().id, "through", camera);
    context_.logger->get()->info("{} looks through camera {}", view->name(), camera);
}

/**
 **/
void ViewCommands::toggleLock(const Context::Placed& at, view::ViewPort* view) {
    view::ViewPort::Through through = view->through();
    through.locked = !through.locked;
    view->through(through);
    at.layout->set(at.layout->areas()[at.area].editor().id, "locked", through.locked);
}

/**
 **/
std::optional<std::pair<std::int64_t, std::optional<int>>> ViewCommands::named(const v3d::event::Event& event) {
    const boost::optional<v3d::event::EventData> data = event.data();
    if (data && std::holds_alternative<int>(*data)) {
        return std::pair<std::int64_t, std::optional<int>>(std::get<int>(*data), std::nullopt);
    }
    if (!data || !std::holds_alternative<std::string>(*data)) {
        return std::nullopt;
    }
    const std::string& text = std::get<std::string>(*data);
    const std::size_t at = text.find('@');
    const std::optional<std::int64_t> camera = whole(text.substr(0, at));
    if (!camera || *camera <= 0) {
        return std::nullopt;
    }
    if (at == std::string::npos) {
        return std::pair<std::int64_t, std::optional<int>>(*camera, std::nullopt);
    }
    const std::optional<std::int64_t> area = whole(text.substr(at + 1));
    if (!area || *area < 0 || *area > std::numeric_limits<int>::max()) {
        return std::nullopt;
    }
    return std::pair<std::int64_t, std::optional<int>>(*camera, static_cast<int>(*area));
}

/**
 **/
void ViewCommands::toggleProjection(view::ViewPort* view) const {
    const boost::shared_ptr<v3d::type::camera::Camera> camera = view->camera();
    camera->orthographic(!camera->orthographic());
    context_.logger->get()->info("{} is {}", view->name(), camera->orthographic() ? "orthographic" : "perspective");
}

};  // namespace v3d::editor::app
