/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

// The controller's other windows: opening one for an editor, keeping and reopening those a
// workspace holds, drawing and closing them.

#include "Controller.h"

#include <api/asset/Json.h>
#include <api/event/Event.h>
#include <api/event/kind/WindowClose.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Component.h>
#include <api/ui/Immediate.h>
#include <api/ui/Interface.h>
#include <api/ui/input/Cursor.h>
#include <api/ui/shell/Keyboard.h>
#include <api/ui/shell/Screen.h>
#include <api/window/Window.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/control/chrome/Area.h>
#include <vertical3d/src/control/chrome/Pointer.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/ViewOverlay.h>
#include <vertical3d/src/view/ViewPort.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Renderer.h"

#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 **/
bool Controller::shows(std::string_view kind) const {
    return std::ranges::any_of(context_.layout->areas(), [kind](const view::AreaLayout::Area& area) { return area.editor().kind == kind; }) ||
        std::ranges::any_of(opened_, [kind](const Opened& each) {
            return std::ranges::any_of(each.layout->areas(), [kind](const view::AreaLayout::Area& area) { return area.editor().kind == kind; });
        });
}

/**
 **/
bool Controller::openWindow(std::string_view kind) {
    for (const Opened& opened : opened_) {
        const std::vector<view::AreaLayout::Area>& areas = opened.layout->areas();
        for (std::size_t area = 0; area < areas.size(); area++) {
            const auto found = std::ranges::find(areas[area].editors, kind, &view::AreaLayout::Editor::kind);
            if (found != areas[area].editors.end()) {
                opened.layout->choose(area, static_cast<std::size_t>(found - areas[area].editors.begin()));
                return true;
            }
        }
    }
    const boost::json::object layout{ { "root", boost::json::object{ { "editors", boost::json::array{ boost::json::object{ { "kind", kind } } } } } } };
    return openWindow(layout, glm::ivec2(OPENED_WIDTH, OPENED_HEIGHT));
}

/**
 **/
bool Controller::openWindow(const boost::json::object& layout, const glm::ivec2& size) {
    Opened opened;
    opened.layout = boost::make_shared<view::AreaLayout>(logger(), nextOpened_);
    if (!opened.layout->load(layout)) {
        return false;
    }
    opened.window = addWindow(size);
    if (!opened.window) {
        return false;
    }
    nextOpened_ += OPENED_IDS;
    opened.layout->metrics(context_.layout->metrics());
    opened.views = boost::make_shared<view::AreaViews>(opened.layout, [this](const view::AreaLayout::Editor& editor) { return makeView(editor); });
    opened.views->sync();
    opened.ui = boost::make_shared<v3d::ui::Interface>(events(), dispatcher(), logger());
    opened.ui->shareThemes(*shell_.ui());
    opened.chrome = boost::make_shared<control::chrome::Area>(opened.ui, std::string(),
        [this](const boost::shared_ptr<v3d::ui::Component>& component) { renderer_->resolve(component); });
    const boost::shared_ptr<v3d::ui::input::Cursor> cursor = boost::make_shared<v3d::ui::input::Cursor>(opened.ui, dispatcher(), renderer_->type());
    // the show flags a check item draws are the active view's, in whichever window it is
    opened.pointer = boost::make_shared<control::chrome::Pointer>(cursor, opened.views, tools_, [this](const boost::shared_ptr<view::ViewPort>&) { syncUi(); });
    const boost::shared_ptr<control::chrome::Area> chrome = opened.chrome;
    opened.pointer->tabs([chrome](const glm::vec2& point) { return chrome->tabAt(point); });
    // a press on a ball of a view's gizmo turns its camera to look down that axis
    const boost::shared_ptr<view::AreaViews> views = opened.views;
    opened.pointer->controls([this, views](const glm::vec2& point) {
        const boost::shared_ptr<view::ViewPort> view = views->of(views->layout()->areaAt(point.x, point.y));
        const std::optional<view::AxisEnd> end = view ? gizmoAt(*view, renderer_->overlay(), point) : std::nullopt;
        if (!end) {
            return false;
        }
        turn(*view, *end);
        syncUi();
        return true;
    });
    if (!renderer_->open(opened.window, { .ui = opened.ui, .views = opened.views, .pointer = opened.pointer })) {
        removeWindow(opened.window);
        return false;
    }
    opened.window->caption("Vertical|3D");
    opened.keys = boost::make_shared<v3d::ui::shell::Keyboard>(opened.ui, dispatcher(), opened.window);
    if (v3d::ui::shell::Screen* screen = renderer_->screen(opened.window)) {
        opened.keys->immediate(screen->immediate());
    }
    opened.pointer->resize(glm::vec4(0.0F, 0.0F, static_cast<float>(size.x), static_cast<float>(size.y)));
    context_.others.push_back({ .layout = opened.layout, .views = opened.views });
    opened_.push_back(opened);
    return true;
}

/**
 **/
boost::json::array Controller::keptWindows() const {
    boost::json::array kept;
    for (const Opened& opened : opened_) {
        kept.emplace_back(boost::json::object{ { "width", opened.window->width() }, { "height", opened.window->height() },
            { "layout", opened.layout->save() } });
    }
    return kept;
}

/**
 **/
void Controller::reopenWindows(const boost::json::array& windows) {
    if (!renderer_ || !chrome_) {
        pendingWindows_ = windows;
        return;
    }
    pendingWindows_.clear();
    while (!opened_.empty()) {
        closeWindow(opened_.back().window->id());
    }
    for (const boost::json::value& entry : windows) {
        const boost::json::object* window = entry.if_object();
        const boost::json::object* layout = window != nullptr ? v3d::asset::readObject(*window, "layout") : nullptr;
        const std::optional<int> width = window != nullptr ? v3d::asset::readInt(*window, "width") : std::nullopt;
        const std::optional<int> height = window != nullptr ? v3d::asset::readInt(*window, "height") : std::nullopt;
        // a window too small to hold an area, or larger than any display, opens at its first size
        const auto side = [](const std::optional<int>& kept, int first) {
            return kept && *kept >= MIN_OPENED && *kept <= MAX_OPENED ? *kept : first;
        };
        if (layout == nullptr || !openWindow(*layout, glm::ivec2(side(width, OPENED_WIDTH), side(height, OPENED_HEIGHT)))) {
            logger()->get()->warn("A workspace's window could not be opened again");
        }
    }
}

/**
 **/
void Controller::closeWindow(std::uint32_t window) {
    const auto found = std::ranges::find_if(opened_, [window](const Opened& each) { return each.window->id() == window; });
    if (found == opened_.end()) {
        return;
    }
    // the keyboard lets go of the screen's layer before the screen goes
    found->keys.reset();
    const Opened closed = std::move(*found);
    opened_.erase(found);
    std::erase_if(context_.others, [&closed](const app::Context::Areas& each) { return each.layout == closed.layout; });
    renderer_->close(closed.window);
    removeWindow(closed.window);
}

/**
 **/
void Controller::syncOpened() {
    for (const Opened& opened : opened_) {
        const glm::vec4 region(0.0F, 0.0F, static_cast<float>(opened.window->width()), static_cast<float>(opened.window->height()));
        if (opened.layout->region() != region) {
            opened.pointer->resize(region);
        }
        opened.chrome->restoreKey(shell_.screen()->shortcut(v3d::event::Event("area::maximise")));
        opened.chrome->sync(*opened.views, context_.selectTool->mask(), renderer_->barHeight(), opened.pointer->activeArea(), renderer_->frameLine());
    }
}

/**
 **/
void Controller::drawOpened(v3d::ui::Immediate* layer, v3d::render::realtime::canvas::Canvas* canvas, std::uint32_t window) {
    const auto found = std::ranges::find_if(opened_, [window](const Opened& each) { return each.window->id() == window; });
    if (found != opened_.end()) {
        drawAreas(layer, canvas, *found->layout, found->pointer.get(), window);
    }
}

/**
 **/
void Controller::shapePointer(v3d::window::Window* window, const control::chrome::Pointer& pointer) {
    const std::optional<view::AreaLayout::Divider> lit = pointer.lit();
    if (!lit) {
        window->pointer(v3d::window::Window::Pointer::Arrow);
        return;
    }
    // a divider between stacked areas moves up and down
    window->pointer(lit->across ? v3d::window::Window::Pointer::Rows : v3d::window::Window::Pointer::Columns);
}

/**
 **/
bool Controller::ownWindow(std::uint32_t window) const {
    return window == 0 || !this->window() || window == this->window()->id();
}

/**
 **/
void Controller::handleClose(const v3d::event::kind::WindowClose& event) {
    closeWindow(event.window());
}

};  // namespace v3d::editor
