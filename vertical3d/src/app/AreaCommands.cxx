/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "AreaCommands.h"

#include <api/asset/Json.h>
#include <api/event/Commands.h>
#include <api/event/Event.h>
#include <api/event/State.h>
#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/control/chrome/Area.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <boost/json.hpp>
#include <boost/optional/optional.hpp>

namespace v3d::editor::app {

/**
 **/
AreaCommands::AreaCommands(const Context& context) :
    context_(context) {
}

/**
 **/
void AreaCommands::registerCommands(CommandTable* table) const {
    registerEdges(table);
    registerTabs(table);
    change(table, "area::maximise", [this](const v3d::event::Event& event) {
        if (event.state() != v3d::event::State::Released) {
            context_.layout->maximise(context_.areaOf(event));
        }
    });
}

/**
 **/
void AreaCommands::change(CommandTable* table, const std::string& name, const v3d::event::Commands::Handler& handler) const {
    table->add(name, [this, handler](const v3d::event::Event& event) {
        handler(event);
        context_.areasChanged();
    });
}

/**
 **/
void AreaCommands::registerEdges(CommandTable* table) const {
    const std::pair<const char*, view::AreaLayout::Edge> edges[] = {
        { "left", view::AreaLayout::Edge::Left }, { "right", view::AreaLayout::Edge::Right },
        { "top", view::AreaLayout::Edge::Top }, { "bottom", view::AreaLayout::Edge::Bottom },
    };
    for (const auto& [word, edge] : edges) {
        change(table, std::string("area::split::") + word, [this, edge](const v3d::event::Event& event) {
            context_.layout->split(context_.areaOf(event), edge);
        });
        change(table, std::string("area::join::") + word, [this, edge](const v3d::event::Event& event) {
            const std::size_t area = context_.areaOf(event);
            context_.layout->join(area, context_.layout->neighbour(area, edge));
        });
        change(table, std::string("area::swap::") + word, [this, edge](const v3d::event::Event& event) {
            const std::size_t area = context_.areaOf(event);
            context_.layout->swap(area, context_.layout->neighbour(area, edge));
        });
    }
}

/**
 **/
void AreaCommands::registerTabs(CommandTable* table) const {
    const Context& context = context_;
    change(table, "area::tab::new", [&context](const v3d::event::Event& event) {
        const std::size_t area = context.areaOf(event);
        if (area >= context.layout->areas().size()) {
            return;
        }
        // a new tab shows the camera the area shows, or the perspective view
        const view::AreaLayout::Editor& shown = context.layout->areas()[area].editor();
        const std::optional<std::string> camera = v3d::asset::readString(shown.state, "camera");
        context.layout->add(area, boost::json::object{ { "kind", view::AreaViews::KIND }, { "camera", camera.value_or("Perspective") } });
    });
    for (const control::chrome::Area::Kind& kind : control::chrome::Area::kinds()) {
        const std::string named = kind.kind;
        change(table, "area::editor::" + named, [&context, named](const v3d::event::Event& event) {
            const std::size_t area = context.areaOf(event);
            if (area >= context.layout->areas().size() || context.layout->areas()[area].editor().kind == named) {
                return;
            }
            // a viewport made this way starts from the perspective view
            boost::json::object state{ { "kind", named } };
            if (named == view::AreaViews::KIND) {
                state["camera"] = "Perspective";
            }
            context.layout->replace(area, state);
        });
    }
    change(table, "area::tab::close", [&context](const v3d::event::Event& event) {
        const std::size_t area = context.areaOf(event);
        if (area < context.layout->areas().size()) {
            context.layout->close(area, context.layout->areas()[area].chosen);
        }
    });
    change(table, "area::tab::choose", [&context](const v3d::event::Event& event) {
        const boost::optional<v3d::event::EventData> data = event.data();
        if (!data || !std::holds_alternative<int>(*data)) {
            return;
        }
        const std::size_t editor = static_cast<std::size_t>(std::get<int>(*data));
        for (std::size_t area = 0; area < context.layout->areas().size(); area++) {
            const std::vector<view::AreaLayout::Editor>& editors = context.layout->areas()[area].editors;
            const auto found = std::ranges::find(editors, editor, &view::AreaLayout::Editor::id);
            if (found != editors.end()) {
                context.layout->choose(area, static_cast<std::size_t>(found - editors.begin()));
                return;
            }
        }
    });
}

};  // namespace v3d::editor::app
