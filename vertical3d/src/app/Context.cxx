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
        for (const boost::shared_ptr<view::AreaLayout>& other : others) {
            const std::size_t found = other->find(id);
            if (found < other->areas().size()) {
                return { .layout = other, .area = found, .main = false };
            }
        }
        return { .layout = layout, .area = layout->find(id), .main = true };
    }
    return { .layout = layout, .area = pointer ? pointer->activeArea() : layout->areas().size(), .main = true };
}

/**
 **/
std::optional<Context::Placed> Context::editorOf(std::size_t editor) const {
    const auto in = [editor](const boost::shared_ptr<view::AreaLayout>& held, bool main) -> std::optional<Placed> {
        for (std::size_t area = 0; area < held->areas().size(); area++) {
            const std::vector<view::AreaLayout::Editor>& editors = held->areas()[area].editors;
            if (std::ranges::find(editors, editor, &view::AreaLayout::Editor::id) != editors.end()) {
                return Placed{ .layout = held, .area = area, .main = main };
            }
        }
        return std::nullopt;
    };
    if (const std::optional<Placed> found = in(layout, true)) {
        return found;
    }
    for (const boost::shared_ptr<view::AreaLayout>& other : others) {
        if (const std::optional<Placed> found = in(other, false)) {
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
void Context::masked(selection::SelectMask mask, const std::string& name, const std::function<document::Outcome()>& operation) const {
    if (selectTool->mask() != mask) {
        refuse(name + " needs the " + selectMaskName(mask) + " select mask");
        return;
    }
    report(operation());
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
