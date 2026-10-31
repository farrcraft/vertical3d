/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "AreaChrome.h"

#include <api/asset/Json.h>
#include <api/event/Event.h>
#include <api/ui/Component.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/Layout.h>
#include <api/ui/Length.h>
#include <api/ui/component/Button.h>
#include <vertical3d/src/view/AreaLayout.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <boost/json.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

namespace {

/**
 * The four ways an area can be divided or met: as each command names it, and as a menu reads it
 * for a split and for a join or a swap.
 **/
struct Way final {
    const char* command;
    const char* split;
    const char* across;
};

constexpr Way WAYS[] = {
    { .command = "left", .split = "Left", .across = "Left" },
    { .command = "right", .split = "Right", .across = "Right" },
    { .command = "top", .split = "Top", .across = "Above" },
    { .command = "bottom", .split = "Bottom", .across = "Below" },
};

/**
 * @return a menu item that sends an area's command
 **/
boost::json::object item(const std::string& label, const std::string& command, std::size_t area) {
    return boost::json::object{ { "label", label }, { "type", "action" }, { "context", AreaChrome::CONTEXT },
        { "command", command }, { "data", static_cast<std::int64_t>(area) } };
}

/**
 * @return a kind with a capital, as a menu names it
 **/
std::string capital(const std::string& kind) {
    std::string named = kind;
    if (!named.empty()) {
        named.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(named.front())));
    }
    return named;
}

/** A submenu of one of an area's commands, in each of the four ways. **/
struct Ways final {
    const char* label;
    const char* command;
    bool across;   /**< whether the ways read as neighbours rather than as edges **/
};

/**
 * @return the submenu, its items sending the command for one area
 **/
boost::json::object ways(const Ways& spec, std::size_t area) {
    boost::json::array items;
    for (const Way& way : WAYS) {
        items.emplace_back(item(spec.across ? way.across : way.split, std::string(spec.command) + "::" + way.command, area));
    }
    return boost::json::object{ { "label", spec.label }, { "type", "submenu" }, { "items", std::move(items) } };
}

/**
 * Set a component's box in window pixels.
 **/
void box(v3d::ui::Component* component, const glm::vec4& region) {
    v3d::ui::Layout& layout = component->layout();
    layout.x = v3d::ui::Length(region.x, v3d::ui::Length::Unit::Pixels);
    layout.y = v3d::ui::Length(region.y, v3d::ui::Length::Unit::Pixels);
    layout.width = v3d::ui::Length(std::max(0.0F, region.z), v3d::ui::Length::Unit::Pixels);
    layout.height = v3d::ui::Length(std::max(0.0F, region.w), v3d::ui::Length::Unit::Pixels);
}

};  // namespace

/**
 **/
AreaChrome::AreaChrome(const boost::shared_ptr<v3d::ui::Interface>& ui, const std::string& before, const Resolve& resolve) :
    ui_(ui),
    before_(before),
    resolve_(resolve) {
}

/**
 **/
std::string AreaChrome::name(std::size_t area) {
    return "area-" + std::to_string(area);
}

/**
 **/
std::string AreaChrome::label(const AreaLayout::Editor& editor) {
    if (const std::optional<std::string> camera = v3d::asset::readString(editor.state, "camera")) {
        return *camera;
    }
    return capital(editor.kind);
}

/**
 **/
std::string AreaChrome::signature(const AreaLayout& layout) {
    std::string made;
    for (const AreaLayout::Area& area : layout.areas()) {
        made += std::to_string(area.id) + ":" + area.editor().kind + ":";
        for (const AreaLayout::Editor& editor : area.editors) {
            made += std::to_string(editor.id) + "=" + label(editor) + ",";
        }
        made += ";";
    }
    return made;
}

/**
 **/
boost::json::object AreaChrome::document(const AreaLayout& layout) {
    boost::json::array containers;
    for (const AreaLayout::Area& area : layout.areas()) {
        const std::size_t id = area.id;
        boost::json::array items = {
            ways({ .label = "Split", .command = "split", .across = false }, id),
            ways({ .label = "Join", .command = "join", .across = true }, id),
            ways({ .label = "Swap", .command = "swap", .across = true }, id),
            boost::json::object{ { "type", "separator" } },
            item("New Tab", "tab::new", id),
            item("Close Tab", "tab::close", id),
            boost::json::object{ { "type", "separator" } },
            item("Maximise", "maximise", id),
        };
        boost::json::array menus = { boost::json::object{ { "label", capital(area.editor().kind) }, { "items", std::move(items) } } };
        boost::json::array components = {
            boost::json::object{ { "type", "menubar" }, { "name", "header" }, { "docked", false }, { "menus", std::move(menus) } },
            boost::json::object{ { "type", "button" }, { "name", "maximise" }, { "icon", "icons/maximise.png" },
                { "tooltip", "Maximise" }, { "context", CONTEXT }, { "command", "maximise" }, { "data", static_cast<std::int64_t>(id) } },
        };
        if (area.editors.size() >= 2) {
            boost::json::array tabs;
            for (const AreaLayout::Editor& editor : area.editors) {
                tabs.emplace_back(boost::json::object{ { "type", "button" }, { "name", "tab-" + std::to_string(editor.id) },
                    { "label", label(editor) }, { "toggle", true }, { "context", CONTEXT }, { "command", "tab::choose" },
                    { "data", static_cast<std::int64_t>(editor.id) } });
            }
            components.emplace_back(boost::json::object{ { "type", "hbox" }, { "name", "tabs" }, { "children", std::move(tabs) } });
        }
        containers.emplace_back(boost::json::object{ { "name", name(id) }, { "visible", true }, { "components", std::move(components) } });
    }
    return boost::json::object{ { "containers", std::move(containers) } };
}

/**
 **/
void AreaChrome::sync(const AreaLayout& layout, float height) {
    const std::string now = signature(layout);
    if (now != signature_) {
        for (const std::string& held : held_) {
            ui_->remove(held);
        }
        held_.clear();
        signature_ = now;
        if (ui_->add(document(layout), before_)) {
            for (const AreaLayout::Area& area : layout.areas()) {
                held_.push_back(name(area.id));
                if (resolve_) {
                    for (const boost::shared_ptr<v3d::ui::Component>& component : ui_->container(held_.back())->components()) {
                        resolve_(component);
                    }
                }
            }
        }
    }
    for (const AreaLayout::Area& area : layout.areas()) {
        place(area, height);
    }
}

/**
 **/
void AreaChrome::place(const AreaLayout::Area& area, float height) const {
    const boost::shared_ptr<v3d::ui::Container> container = ui_->container(name(area.id));
    if (!container) {
        return;
    }
    const glm::vec4& content = area.content;
    const bool roomy = content.z > height && content.w > height;
    container->visible(roomy);
    if (!roomy) {
        return;
    }
    if (const boost::shared_ptr<v3d::ui::Component> header = container->get("header")) {
        box(header.get(), glm::vec4(content.x, content.y, content.z - height, height));
    }
    if (const boost::shared_ptr<v3d::ui::Component> maximise = container->get("maximise")) {
        box(maximise.get(), glm::vec4(content.x + content.z - height, content.y, height, height));
    }
    if (const boost::shared_ptr<v3d::ui::Component> tabs = container->get("tabs")) {
        box(tabs.get(), glm::vec4(area.region.x, area.region.y, area.region.z, content.y - area.region.y));
        for (std::size_t index = 0; index < area.editors.size(); index++) {
            const boost::shared_ptr<v3d::ui::component::Button> tab = boost::dynamic_pointer_cast<v3d::ui::component::Button>(
                container->get("tab-" + std::to_string(area.editors[index].id)));
            if (tab) {
                tab->checked(index == area.chosen);
            }
        }
    }
}

/**
 **/
std::optional<std::size_t> AreaChrome::tabAt(const glm::vec2& point) const {
    for (const std::string& held : held_) {
        const boost::shared_ptr<v3d::ui::Container> container = ui_->container(held);
        const boost::shared_ptr<v3d::ui::Component> tabs = container ? container->get("tabs") : nullptr;
        if (!tabs || !container->visible()) {
            continue;
        }
        for (const boost::shared_ptr<v3d::ui::Component>& child : tabs->children()) {
            const boost::shared_ptr<v3d::ui::component::Button> tab = boost::dynamic_pointer_cast<v3d::ui::component::Button>(child);
            if (!tab) {
                continue;
            }
            const glm::vec2 min = tab->position();
            const glm::vec2 max = min + tab->size();
            const boost::optional<v3d::event::EventData> data = tab->event().data();
            if (point.x >= min.x && point.x < max.x && point.y >= min.y && point.y < max.y && data && std::holds_alternative<int>(*data)) {
                return static_cast<std::size_t>(std::get<int>(*data));
            }
        }
    }
    return std::nullopt;
}

};  // namespace v3d::editor
