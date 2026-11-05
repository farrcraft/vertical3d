/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Context.h"

#include <api/event/Event.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/render/Shot.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/script/Arguments.h>
#include <vertical3d/src/script/Journal.h>
#include <vertical3d/src/script/Text.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/ViewPort.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/json.hpp>
#include <boost/optional/optional.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor::app {

/**
 **/
void Context::report(const document::Outcome& outcome) const {
    if (!outcome.message.empty()) {
        logger->get()->info("{}", outcome.message);
        if (notices) {
            notices->post({ .text = outcome.message, .done = outcome.done, .undoable = outcome.undoable });
        }
        if (journal) {
            journal->add(outcome.done ? script::Journal::Kind::Done : script::Journal::Kind::Refused, outcome.message);
        }
    }
}

/**
 **/
void Context::refuse(const std::string& message) const {
    report({ .done = false, .message = message });
}

/**
 **/
boost::shared_ptr<view::ViewPort> Context::activeView() const {
    return pointer ? pointer->activeView() : nullptr;
}

/**
 **/
std::size_t Context::areaOf(const v3d::event::Event& event) const {
    const boost::optional<v3d::event::EventData> data = event.data();
    if (data && std::holds_alternative<int>(*data)) {
        return layout->find(static_cast<std::size_t>(std::get<int>(*data)));
    }
    return pointer ? pointer->activeArea() : layout->areas().size();
}

/**
 **/
Context::Placed Context::placeOf(const v3d::event::Event& event) const {
    const boost::optional<v3d::event::EventData> data = event.data();
    if (data && std::holds_alternative<int>(*data)) {
        const std::size_t id = static_cast<std::size_t>(std::get<int>(*data));
        for (const Areas& other : others) {
            const std::size_t found = other.layout->find(id);
            if (found < other.layout->areas().size()) {
                return { .layout = other.layout, .views = other.views, .area = found };
            }
        }
        return { .layout = layout, .views = views, .area = layout->find(id) };
    }
    if (keyed) {
        if (const std::optional<Placed> there = keyed()) {
            return *there;
        }
    }
    return { .layout = layout, .views = views, .area = pointer ? pointer->activeArea() : layout->areas().size() };
}

/**
 **/
std::optional<Context::Placed> Context::editorOf(std::size_t editor) const {
    const auto in = [editor](const Areas& held) -> std::optional<Placed> {
        for (std::size_t area = 0; area < held.layout->areas().size(); area++) {
            const std::vector<view::AreaLayout::Editor>& editors = held.layout->areas()[area].editors;
            if (std::ranges::find(editors, editor, &view::AreaLayout::Editor::id) != editors.end()) {
                return Placed{ .layout = held.layout, .views = held.views, .area = area };
            }
        }
        return std::nullopt;
    };
    if (const std::optional<Placed> found = in({ .layout = layout, .views = views })) {
        return found;
    }
    for (const Areas& other : others) {
        if (const std::optional<Placed> found = in(other)) {
            return found;
        }
    }
    return std::nullopt;
}

/**
 **/
void Context::show(std::string_view kind, bool windowed) const {
    const std::vector<view::AreaLayout::Area>& areas = layout->areas();
    for (std::size_t area = 0; area < areas.size(); area++) {
        const std::vector<view::AreaLayout::Editor>& editors = areas[area].editors;
        const auto found = std::ranges::find(editors, kind, &view::AreaLayout::Editor::kind);
        if (found != editors.end()) {
            layout->choose(area, static_cast<std::size_t>(found - editors.begin()));
            areasChanged();
            return;
        }
    }
    if (windowed && window && window(kind)) {
        return;
    }
    const std::size_t active = pointer ? pointer->activeArea() : 0;
    if (layout->add(active, boost::json::object{ { "kind", kind } })) {
        areasChanged();
    }
}

/**
 **/
void Context::atView(const std::string& what, const std::function<void(const v3d::type::camera::Profile&)>& run) const {
    const boost::shared_ptr<view::ViewPort> view = activeView();
    if (!view || !view->camera()) {
        refuse(what + " at a view, and there is no active view");
        return;
    }
    run(view->camera()->profile());
}

/**
 **/
void Context::masked(script::Arguments* arguments, selection::SelectMask mask, const std::string& name,
    const std::function<document::Outcome()>& operation) const {
    const boost::shared_ptr<scene::Scene> scene = document->scene();
    if (arguments->has("parts")) {
        if (const std::optional<std::string> problem = script::select(scene.get(), arguments->names("parts", {}))) {
            refuse(name + ": " + *problem);
            return;
        }
    } else {
        if (selectTool->mask() != mask) {
            refuse(name + " needs the " + selectMaskName(mask) + " select mask");
            return;
        }
        arguments->names("parts", [&scene]() { return script::selectedParts(*scene, scene->selection()); });
    }
    report(operation());
}

/**
 **/
std::optional<scene::Items> Context::items(script::Arguments* arguments, std::string_view parameter, const std::string& what) const {
    const boost::shared_ptr<scene::Scene> scene = document->scene();
    scene::Items found;
    const std::vector<std::string> named = arguments->names(parameter, [&scene]() { return script::names(scene->selection()); });
    if (const std::optional<std::string> problem = script::resolve(*scene, named, &found)) {
        refuse(what + ": " + *problem);
        return std::nullopt;
    }
    return found;
}

/**
 **/
v3d::type::camera::Profile Context::shot() const {
    return render::shotFrom(*document->scene(), pointer->activeView()->camera()->profile());
}

/**
 **/
std::string Context::projectKey() const {
    if (!document) {
        return {};
    }
    return boost::filesystem::absolute(document->project()->directory()).lexically_normal().generic_string();
}

};  // namespace v3d::editor::app
