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
#include <api/type/camera/Camera.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/menu/Bar.h>
#include <api/ui/component/menu/Item.h>
#include <vertical3d/src/control/Outliner.h>
#include <vertical3d/src/control/PropertiesEditor.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/Shading.h>
#include <vertical3d/src/view/ViewPort.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <boost/json.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 **/
AreaChrome::AreaChrome(const boost::shared_ptr<v3d::ui::Interface>& ui, const std::string& before, const Resolve& resolve) :
    ui_(ui),
    before_(before),
    resolve_(resolve) {
}

/**
 **/
boost::json::object AreaChrome::item(const std::string& label, const std::string& command, std::size_t area) {
    return boost::json::object{ { "label", label }, { "type", "action" }, { "context", CONTEXT },
        { "command", command }, { "data", static_cast<std::int64_t>(area) } };
}

/**
 **/
boost::json::object AreaChrome::mark(const std::string& label, const char* type, const std::string& command, std::size_t area) {
    return boost::json::object{ { "label", label }, { "type", type }, { "context", VIEW_CONTEXT },
        { "command", command }, { "data", static_cast<std::int64_t>(area) } };
}

/**
 **/
boost::json::object AreaChrome::choices(const char* label, const std::string& prefix, std::span<const Choice> each, std::size_t area) {
    boost::json::array items;
    for (const Choice& choice : each) {
        items.emplace_back(mark(choice.label, "radio", prefix + choice.command, area));
    }
    return boost::json::object{ { "label", label }, { "type", "submenu" }, { "items", std::move(items) } };
}

/**
 **/
std::string AreaChrome::capital(const std::string& kind) {
    std::string named = kind;
    if (!named.empty()) {
        named.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(named.front())));
    }
    return named;
}

/**
 **/
boost::json::object AreaChrome::maskMenu(SelectMask mask, std::size_t area) {
    boost::json::array items;
    for (const Masked& each : MASKS) {
        if (each.rule) {
            items.emplace_back(boost::json::object{ { "type", "separator" } });
        }
        const std::string name = selectMaskName(each.mask);
        items.emplace_back(boost::json::object{ { "label", capital(name) }, { "type", "radio" }, { "context", "select" },
            { "command", "mask::" + name }, { "data", static_cast<std::int64_t>(area) } });
    }
    return boost::json::object{ { "label", capital(selectMaskName(mask)) }, { "items", std::move(items) } };
}

/**
 **/
boost::json::array AreaChrome::viewMenus(const ViewPort& view, std::size_t area) {
    boost::json::array cameras;
    for (const FixedView& fixed : fixedViews()) {
        // an item names its command without its context
        cameras.emplace_back(mark(fixed.profile, "radio", fixed.command.substr(VIEW_CONTEXT.size() + 2), area));
    }
    cameras.emplace_back(boost::json::object{ { "type", "separator" } });
    cameras.emplace_back(mark("Orthographic", "check", "camera::orthographic", area));

    boost::json::array shading = {
        mark("Flat", "radio", "shade::flat", area),
        mark("Smooth", "radio", "shade::smooth", area),
        boost::json::object{ { "type", "separator" } },
        choices("Front Facing", "rendermode::front::", FILLS, area),
        choices("Back Facing", "rendermode::back::", FILLS, area),
        choices("Cull", "cull::", CULLS, area),
    };
    return boost::json::array{
        boost::json::object{ { "label", view.camera()->profile().name() }, { "items", std::move(cameras) } },
        boost::json::object{ { "label", AreaChrome::shading(view.display()) }, { "items", std::move(shading) } },
    };
}

/**
 **/
boost::json::object AreaChrome::ways(const Ways& spec, std::size_t area) {
    boost::json::array items;
    for (const Way& way : WAYS) {
        items.emplace_back(item(spec.across ? way.across : way.split, std::string(spec.command) + "::" + way.command, area));
    }
    return boost::json::object{ { "label", spec.label }, { "type", "submenu" }, { "items", std::move(items) } };
}

/**
 **/
void AreaChrome::check(v3d::ui::component::menu::Bar* bar, const std::vector<std::pair<std::string, bool>>& marks) {
    for (const auto& [command, on] : marks) {
        if (const boost::shared_ptr<v3d::ui::component::menu::Item> found = bar->find(command)) {
            found->checked(on);
        }
    }
}

/**
 **/
void AreaChrome::box(v3d::ui::Component* component, const glm::vec4& region) {
    v3d::ui::Layout& layout = component->layout();
    layout.x = v3d::ui::Length(region.x, v3d::ui::Length::Unit::Pixels);
    layout.y = v3d::ui::Length(region.y, v3d::ui::Length::Unit::Pixels);
    layout.width = v3d::ui::Length(std::max(0.0F, region.z), v3d::ui::Length::Unit::Pixels);
    layout.height = v3d::ui::Length(std::max(0.0F, region.w), v3d::ui::Length::Unit::Pixels);
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
const std::vector<AreaChrome::FixedView>& AreaChrome::fixedViews() {
    static const std::vector<FixedView> views{
        { .command = "view::camera::front", .profile = "Front" },
        { .command = "view::camera::back", .profile = "Back" },
        { .command = "view::camera::left", .profile = "Left" },
        { .command = "view::camera::right", .profile = "Right" },
        { .command = "view::camera::top", .profile = "Top" },
        { .command = "view::camera::bottom", .profile = "Bottom" },
        { .command = "view::camera::perspective", .profile = "Perspective" },
    };
    return views;
}

/**
 **/
std::string AreaChrome::shading(const ViewPort::Display& display) {
    switch (display.front) {
        case ViewPort::Fill::Points:
            return "Points";
        case ViewPort::Fill::Lines:
            return "Wireframe";
        case ViewPort::Fill::Fill:
            break;
    }
    return display.shading == Shading::Smooth ? "Smooth" : "Flat";
}

/**
 **/
std::vector<std::pair<std::string, bool>> AreaChrome::marks(const ViewPort& view) {
    std::vector<std::pair<std::string, bool>> made{
        { "view::show::mesh", view.shows(ViewPort::SHOW_MESH) },
        { "view::show::camera", view.shows(ViewPort::SHOW_CAMERA) },
        { "view::show::light", view.shows(ViewPort::SHOW_LIGHT) },
        { "view::show::handle", view.shows(ViewPort::SHOW_HANDLE) },
        { "view::show::grid", view.shows(ViewPort::SHOW_GRID) },
    };
    const ViewPort::Display& shown = view.display();
    made.emplace_back("view::shade::flat", shown.shading == Shading::Flat);
    made.emplace_back("view::shade::smooth", shown.shading == Shading::Smooth);
    for (std::size_t index = 0; index < std::size(CULLS); index++) {
        made.emplace_back(std::string("view::cull::") + CULLS[index].command, shown.cull == CULLED[index]);
    }
    for (std::size_t index = 0; index < std::size(FILLS); index++) {
        made.emplace_back(std::string("view::rendermode::front::") + FILLS[index].command, shown.front == FILLED[index]);
        made.emplace_back(std::string("view::rendermode::back::") + FILLS[index].command, shown.back == FILLED[index]);
    }
    // a view laid out from a profile no fixed view is, such as a second perspective, marks none
    const v3d::type::camera::Camera& camera = *view.camera();
    for (const FixedView& fixed : fixedViews()) {
        made.emplace_back(fixed.command, camera.profile().name() == fixed.profile);
    }
    made.emplace_back("view::camera::orthographic", camera.orthographic());
    return made;
}

/**
 **/
const std::vector<AreaChrome::Kind>& AreaChrome::kinds() {
    static const std::vector<Kind> each{
        { .kind = std::string(AreaViews::KIND), .label = "Viewport" },
        { .kind = std::string(Outliner::KIND), .label = "Outliner" },
        { .kind = std::string(PropertiesEditor::KIND), .label = "Properties" },
    };
    return each;
}

/**
 **/
std::vector<std::pair<std::string, bool>> AreaChrome::marks(const AreaLayout::Editor& editor) {
    std::vector<std::pair<std::string, bool>> made;
    for (const Kind& kind : kinds()) {
        made.emplace_back("area::editor::" + kind.kind, kind.kind == editor.kind);
    }
    return made;
}

/**
 **/
std::vector<std::pair<std::string, bool>> AreaChrome::marks(SelectMask mask) {
    std::vector<std::pair<std::string, bool>> made;
    for (const Masked& each : MASKS) {
        made.emplace_back(std::string("select::mask::") + selectMaskName(each.mask), each.mask == mask);
    }
    return made;
}

/**
 **/
std::string AreaChrome::signature(const AreaViews& views, SelectMask mask) {
    const AreaLayout& layout = *views.layout();
    std::string made = std::string(selectMaskName(mask)) + ";";
    for (std::size_t index = 0; index < layout.areas().size(); index++) {
        const AreaLayout::Area& area = layout.areas()[index];
        made += std::to_string(area.id) + ":" + area.editor().kind + ":";
        for (const AreaLayout::Editor& editor : area.editors) {
            made += std::to_string(editor.id) + "=" + label(editor) + ",";
        }
        if (const boost::shared_ptr<ViewPort> view = views.of(index)) {
            made += view->camera()->profile().name() + "/" + shading(view->display());
        }
        made += ";";
    }
    return made;
}

/**
 **/
boost::json::object AreaChrome::document(const AreaViews& views, SelectMask mask) {
    const AreaLayout& layout = *views.layout();
    boost::json::array containers;
    for (std::size_t index = 0; index < layout.areas().size(); index++) {
        const AreaLayout::Area& area = layout.areas()[index];
        const std::size_t id = area.id;
        boost::json::array kinds;
        for (const Kind& kind : AreaChrome::kinds()) {
            kinds.emplace_back(boost::json::object{ { "label", kind.label }, { "type", "radio" }, { "context", CONTEXT },
                { "command", "editor::" + kind.kind }, { "data", static_cast<std::int64_t>(id) } });
        }
        boost::json::array items = {
            boost::json::object{ { "label", "Editor" }, { "type", "submenu" }, { "items", std::move(kinds) } },
            boost::json::object{ { "type", "separator" } },
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
        if (const boost::shared_ptr<ViewPort> view = views.of(index)) {
            for (boost::json::value& menu : viewMenus(*view, id)) {
                menus.emplace_back(std::move(menu));
            }
            menus.emplace_back(maskMenu(mask, id));
        }
        boost::json::array components = {
            boost::json::object{ { "type", "menubar" }, { "name", "header" }, { "docked", false }, { "menus", std::move(menus) } },
            boost::json::object{ { "type", "button" }, { "name", "maximise" }, { "icon", "icon:maximise" },
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
void AreaChrome::sync(const AreaViews& views, SelectMask mask, float height) {
    const AreaLayout& layout = *views.layout();
    const std::string now = signature(views, mask);
    if (now != signature_) {
        for (const std::string& held : held_) {
            ui_->remove(held);
        }
        held_.clear();
        signature_ = now;
        if (ui_->add(document(views, mask), before_)) {
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
    for (std::size_t index = 0; index < layout.areas().size(); index++) {
        place(layout.areas()[index], views.of(index).get(), mask, height);
    }
}

/**
 **/
void AreaChrome::place(const AreaLayout::Area& area, const ViewPort* view, SelectMask mask, float height) const {
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
        const boost::shared_ptr<v3d::ui::component::menu::Bar> bar = boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(header);
        if (bar) {
            check(bar.get(), marks(area.editor()));
        }
        if (bar && view != nullptr) {
            check(bar.get(), marks(*view));
            check(bar.get(), marks(mask));
        }
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
