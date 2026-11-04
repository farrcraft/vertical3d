/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Area.h"

#include <api/asset/Json.h>
#include <api/event/Event.h>
#include <api/ui/Component.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/Layout.h>
#include <api/ui/Length.h>
#include <api/type/camera/Camera.h>
#include <api/type/geometry/Bound2D.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/menu/Bar.h>
#include <api/ui/component/menu/Item.h>
#include <api/ui/component/menu/Menu.h>
#include <vertical3d/src/control/ConsoleEditor.h>
#include <vertical3d/src/control/GraphEditor.h>
#include <vertical3d/src/control/Outliner.h>
#include <vertical3d/src/control/PropertiesEditor.h>
#include <vertical3d/src/control/RenderSettingsEditor.h>
#include <vertical3d/src/control/RenderView.h>
#include <vertical3d/src/control/TimelineEditor.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/Shading.h>
#include <vertical3d/src/view/Through.h>
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
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control::chrome {

/**
 **/
Area::Area(const boost::shared_ptr<v3d::ui::Interface>& ui, const std::string& before, const Resolve& resolve) :
    ui_(ui),
    before_(before),
    resolve_(resolve) {
}

/**
 **/
boost::json::object Area::item(const std::string& label, const std::string& command, std::size_t area) {
    return boost::json::object{ { "label", label }, { "type", "action" }, { "context", CONTEXT },
        { "command", command }, { "data", static_cast<std::int64_t>(area) } };
}

/**
 **/
boost::json::object Area::mark(const std::string& label, const char* type, const std::string& command, std::size_t area) {
    return boost::json::object{ { "label", label }, { "type", type }, { "context", VIEW_CONTEXT },
        { "command", command }, { "data", static_cast<std::int64_t>(area) } };
}

/**
 **/
boost::json::object Area::choices(const char* label, const std::string& prefix, std::span<const Choice> each, std::size_t area) {
    boost::json::array items;
    for (const Choice& choice : each) {
        items.emplace_back(mark(choice.label, "radio", prefix + choice.command, area));
    }
    return boost::json::object{ { "label", label }, { "type", "submenu" }, { "items", std::move(items) } };
}

/**
 **/
std::string Area::capital(const std::string& kind) {
    std::string named = kind;
    if (!named.empty()) {
        named.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(named.front())));
    }
    return named;
}

/**
 **/
boost::json::object Area::maskMenu(selection::SelectMask mask, std::size_t area) {
    boost::json::array items;
    for (const Masked& each : MASKS) {
        if (each.rule) {
            items.emplace_back(boost::json::object{ { "type", "separator" } });
        }
        const std::string name = selectMaskName(each.mask);
        items.emplace_back(boost::json::object{ { "label", capital(name) }, { "type", "radio" }, { "context", "select" },
            { "command", "mask::" + name }, { "data", static_cast<std::int64_t>(area) } });
    }
    return boost::json::object{ { "label", capital(selectMaskName(mask)) }, { "drop", true }, { "items", std::move(items) } };
}

/**
 **/
boost::json::array Area::viewMenus(const view::ViewPort& view, std::size_t area, const Cameras& scene) {
    boost::json::array cameras;
    for (const FixedView& fixed : fixedViews()) {
        // an item names its command without its context
        cameras.emplace_back(mark(fixed.profile, "radio", fixed.command.substr(VIEW_CONTEXT.size() + 2), area));
    }
    if (!scene.empty()) {
        cameras.emplace_back(boost::json::object{ { "type", "separator" } });
        for (const SceneCamera& camera : scene) {
            cameras.emplace_back(cameraItem(camera, area));
        }
    }
    cameras.emplace_back(boost::json::object{ { "type", "separator" } });
    cameras.emplace_back(mark("Orthographic", "check", "camera::orthographic", area));
    if (!view.free()) {
        cameras.emplace_back(mark("Lock Camera to View", "check", "camera::lock", area));
    }

    boost::json::array shading = {
        mark("Flat", "radio", "shade::flat", area),
        mark("Smooth", "radio", "shade::smooth", area),
        boost::json::object{ { "type", "separator" } },
        choices("Front Facing", "rendermode::front::", FILLS, area),
        choices("Back Facing", "rendermode::back::", FILLS, area),
        choices("Cull", "cull::", CULLS, area),
    };
    return boost::json::array{
        boost::json::object{ { "label", view.camera()->profile().name() }, { "drop", true }, { "items", std::move(cameras) } },
        boost::json::object{ { "label", Area::shading(view.display()) }, { "drop", true }, { "items", std::move(shading) } },
    };
}

/**
 **/
boost::json::object Area::overlaysMenu(std::size_t area) {
    boost::json::array items = {
        mark("Grid", "check", "show::grid", area),
        mark("Meshes", "check", "show::mesh", area),
        mark("Cameras", "check", "show::camera", area),
        mark("Lights", "check", "show::light", area),
        mark("Handles", "check", "show::handle", area),
    };
    return boost::json::object{ { "label", "Overlays" }, { "icon", "icon:overlays" }, { "drop", true }, { "items", std::move(items) } };
}

/**
 **/
boost::json::object Area::ways(const Ways& spec, std::size_t area) {
    boost::json::array items;
    for (const Way& way : WAYS) {
        items.emplace_back(item(spec.across ? way.across : way.split, std::string(spec.command) + "::" + way.command, area));
    }
    return boost::json::object{ { "label", spec.label }, { "type", "submenu" }, { "items", std::move(items) } };
}

/**
 **/
void Area::check(v3d::ui::component::menu::Bar* bar, const std::vector<std::pair<std::string, bool>>& marks) {
    for (const auto& [command, on] : marks) {
        if (const boost::shared_ptr<v3d::ui::component::menu::Item> found = bar->find(command)) {
            found->checked(on);
        }
    }
}

/**
 **/
void Area::box(v3d::ui::Component* component, const glm::vec4& region) {
    v3d::ui::Layout& layout = component->layout();
    layout.x = v3d::ui::Length(region.x, v3d::ui::Length::Unit::Pixels);
    layout.y = v3d::ui::Length(region.y, v3d::ui::Length::Unit::Pixels);
    layout.width = v3d::ui::Length(std::max(0.0F, region.z), v3d::ui::Length::Unit::Pixels);
    layout.height = v3d::ui::Length(std::max(0.0F, region.w), v3d::ui::Length::Unit::Pixels);
}

/**
 **/
std::string Area::name(std::size_t area) {
    return "area-" + std::to_string(area);
}

/**
 **/
std::string Area::label(const view::AreaLayout::Editor& editor) {
    if (const std::optional<std::string> camera = v3d::asset::readString(editor.state, "camera")) {
        return *camera == view::SCENE_CAMERA ? std::string(SCENE_CAMERA_LABEL) : *camera;
    }
    return capital(editor.kind);
}

/**
 **/
std::string Area::kindLabel(const std::string& kind) {
    const std::vector<Kind>& each = kinds();
    const auto found = std::ranges::find(each, kind, &Kind::kind);
    return found == each.end() ? capital(kind) : found->label;
}

/**
 **/
bool Area::headed(const view::AreaLayout::Area& area) {
    return area.editors.size() < 2 || area.editor().kind == view::AreaViews::KIND;
}

/**
 **/
const std::vector<Area::FixedView>& Area::fixedViews() {
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
std::string Area::shading(const view::ViewPort::Display& display) {
    switch (display.front) {
        case view::ViewPort::Fill::Points:
            return "Points";
        case view::ViewPort::Fill::Lines:
            return "Wireframe";
        case view::ViewPort::Fill::Fill:
            break;
    }
    return display.shading == view::Shading::Smooth ? "Smooth" : "Flat";
}

/**
 **/
std::vector<std::pair<std::string, bool>> Area::marks(const view::ViewPort& view) {
    std::vector<std::pair<std::string, bool>> made{
        { "view::show::mesh", view.shows(view::ViewPort::SHOW_MESH) },
        { "view::show::camera", view.shows(view::ViewPort::SHOW_CAMERA) },
        { "view::show::light", view.shows(view::ViewPort::SHOW_LIGHT) },
        { "view::show::handle", view.shows(view::ViewPort::SHOW_HANDLE) },
        { "view::show::grid", view.shows(view::ViewPort::SHOW_GRID) },
    };
    const view::ViewPort::Display& shown = view.display();
    made.emplace_back("view::shade::flat", shown.shading == view::Shading::Flat);
    made.emplace_back("view::shade::smooth", shown.shading == view::Shading::Smooth);
    for (std::size_t index = 0; index < std::size(CULLS); index++) {
        made.emplace_back(std::string("view::cull::") + CULLS[index].command, shown.cull == CULLED[index]);
    }
    for (std::size_t index = 0; index < std::size(FILLS); index++) {
        made.emplace_back(std::string("view::rendermode::front::") + FILLS[index].command, shown.front == FILLED[index]);
        made.emplace_back(std::string("view::rendermode::back::") + FILLS[index].command, shown.back == FILLED[index]);
    }
    // a view laid out from a profile no fixed view is, such as a second perspective, marks none,
    // and so does a view through a scene camera, whatever the camera is called
    const v3d::type::camera::Camera& camera = *view.camera();
    for (const FixedView& fixed : fixedViews()) {
        made.emplace_back(fixed.command, view.free() && camera.profile().name() == fixed.profile);
    }
    made.emplace_back("view::camera::orthographic", camera.orthographic());
    made.emplace_back("view::camera::lock", view.through().locked);
    return made;
}

/**
 **/
const std::vector<Area::Kind>& Area::kinds() {
    static const std::vector<Kind> each{
        { .kind = std::string(view::AreaViews::KIND), .label = "Viewport" },
        { .kind = std::string(Outliner::KIND), .label = "Outliner" },
        { .kind = std::string(PropertiesEditor::KIND), .label = "Properties" },
        { .kind = std::string(TimelineEditor::KIND), .label = "Timeline" },
        { .kind = std::string(GraphEditor::KIND), .label = "Graph Editor" },
        { .kind = std::string(ConsoleEditor::KIND), .label = "Console" },
        { .kind = std::string(RenderView::KIND), .label = "Render View" },
        { .kind = std::string(RenderSettingsEditor::KIND), .label = "Render Settings" },
    };
    return each;
}

/**
 **/
std::vector<std::pair<std::string, bool>> Area::marks(const view::AreaLayout::Editor& editor) {
    std::vector<std::pair<std::string, bool>> made;
    for (const Kind& kind : kinds()) {
        made.emplace_back("area::editor::" + kind.kind, kind.kind == editor.kind);
    }
    return made;
}

/**
 **/
std::vector<std::pair<std::string, bool>> Area::marks(selection::SelectMask mask) {
    std::vector<std::pair<std::string, bool>> made;
    for (const Masked& each : MASKS) {
        made.emplace_back(std::string("select::mask::") + selectMaskName(each.mask), each.mask == mask);
    }
    return made;
}

/**
 **/
std::string Area::signature(const view::AreaViews& views, selection::SelectMask mask, const Cameras& cameras) {
    const view::AreaLayout& layout = *views.layout();
    std::string made = std::string(selectMaskName(mask)) + (layout.maximised() ? ";maximised;" : ";");
    for (const SceneCamera& camera : cameras) {
        made += std::to_string(camera.id) + "=" + camera.name + ",";
    }
    made += ";";
    for (std::size_t index = 0; index < layout.areas().size(); index++) {
        const view::AreaLayout::Area& area = layout.areas()[index];
        made += std::to_string(area.id) + ":" + area.editor().kind + ":";
        for (const view::AreaLayout::Editor& editor : area.editors) {
            made += std::to_string(editor.id) + "=" + label(editor) + ",";
        }
        if (const boost::shared_ptr<view::ViewPort> view = views.of(index)) {
            made += view->camera()->profile().name() + "/" + shading(view->display()) + (view->free() ? "/free" : "/through");
        }
        made += ";";
    }
    return made;
}

/**
 **/
boost::json::object Area::document(const view::AreaViews& views, selection::SelectMask mask, const Cameras& cameras) {
    const view::AreaLayout& layout = *views.layout();
    boost::json::array containers;
    for (std::size_t index = 0; index < layout.areas().size(); index++) {
        const view::AreaLayout::Area& area = layout.areas()[index];
        const std::size_t id = area.id;
        boost::json::array kinds;
        for (const Kind& kind : Area::kinds()) {
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
        const bool tabbed = area.editors.size() >= 2;
        const boost::shared_ptr<view::ViewPort> view = views.of(index);
        // a panel's header names its editor after the menu's icon. A view's names its camera, and
        // a tabbed area's tabs name its editors
        boost::json::object areaMenu{ { "label", kindLabel(area.editor().kind) }, { "icon", "icon:editor-" + area.editor().kind },
            { "drop", true }, { "named", !tabbed && !view }, { "items", std::move(items) } };

        // a tabbed area's strip holds the area's menu, and the row under it holds only what acts
        // on its editor. Otherwise the area's menu leads the header
        boost::json::array components;
        boost::json::array menus;
        if (tabbed) {
            components = strip(area, std::move(areaMenu));
        } else {
            menus.emplace_back(std::move(areaMenu));
        }
        if (view) {
            for (boost::json::value& menu : viewMenus(*view, id, cameras)) {
                menus.emplace_back(std::move(menu));
            }
            menus.emplace_back(maskMenu(mask, id));
            menus.emplace_back(overlaysMenu(id));
        }
        if (!menus.empty()) {
            components.emplace_back(boost::json::object{ { "type", "menubar" }, { "name", "header" }, { "docked", false },
                { "opacity", HEADER_OPACITY }, { "menus", std::move(menus) } });
        }
        // a maximised area offers to restore the layout. place() writes the hint that says how
        const bool maximised = layout.maximised().has_value();
        components.emplace_back(boost::json::object{ { "type", "button" }, { "name", "maximise" },
            { "icon", maximised ? "icon:restore" : "icon:maximise" }, { "tooltip", maximised ? "Restore" : "Maximise" },
            { "context", CONTEXT }, { "command", "maximise" }, { "data", static_cast<std::int64_t>(id) } });
        containers.emplace_back(boost::json::object{ { "name", name(id) }, { "visible", true }, { "components", std::move(components) } });
    }
    return boost::json::object{ { "containers", std::move(containers) } };
}

/**
 **/
boost::json::array Area::strip(const view::AreaLayout::Area& area, boost::json::object areaMenu) {
    const std::size_t id = area.id;
    boost::json::array tabs;
    for (const view::AreaLayout::Editor& editor : area.editors) {
        tabs.emplace_back(boost::json::object{ { "type", "button" }, { "name", "tab-" + std::to_string(editor.id) },
            { "label", label(editor) }, { "toggle", true }, { "tab", "strip" }, { "context", CONTEXT },
            { "command", "tab::choose" }, { "data", static_cast<std::int64_t>(editor.id) } });
    }
    boost::json::array adds;
    for (const Kind& kind : Area::kinds()) {
        adds.emplace_back(item(kind.label, "tab::add::" + kind.kind, id));
    }
    return boost::json::array{
        boost::json::object{ { "type", "panel" }, { "name", "strip" }, { "recess", true } },
        boost::json::object{ { "type", "menubar" }, { "name", "strip-menu" }, { "docked", false }, { "opacity", 0.0 },
            { "menus", boost::json::array{ std::move(areaMenu) } } },
        boost::json::object{ { "type", "hbox" }, { "name", "tabs" }, { "children", std::move(tabs) } },
        boost::json::object{ { "type", "menubar" }, { "name", "add" }, { "docked", false }, { "opacity", 0.0 },
            { "menus", boost::json::array{ boost::json::object{ { "label", "+" }, { "items", std::move(adds) } } } } },
    };
}

/**
 **/
void Area::sync(const view::AreaViews& views, selection::SelectMask mask, float height, std::size_t active) {
    const view::AreaLayout& layout = *views.layout();
    const std::string now = signature(views, mask, cameras_);
    if (now != signature_) {
        for (const std::string& held : held_) {
            ui_->remove(held);
        }
        held_.clear();
        signature_ = now;
        if (ui_->add(document(views, mask, cameras_), before_)) {
            for (const view::AreaLayout::Area& area : layout.areas()) {
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
        place(layout.areas()[index], { .view = views.of(index).get(), .mask = mask, .height = height, .active = index == active,
            .maximised = layout.maximised().has_value() });
    }
}

/**
 **/
Area::Cameras Area::of(const scene::Scene& scene) {
    Cameras made;
    for (const boost::shared_ptr<scene::item::Item>& item : scene.items()) {
        if (item->kind() == scene::item::Item::Kind::Camera) {
            made.push_back({ .id = item->documentId(), .name = item->name() });
        }
    }
    return made;
}

/**
 **/
void Area::cameras(const Cameras& cameras) {
    cameras_ = cameras;
}

/**
 **/
boost::json::object Area::cameraItem(const SceneCamera& camera, std::optional<std::size_t> area) {
    std::string data = std::to_string(camera.id);
    if (area) {
        data += "@" + std::to_string(*area);
    }
    return boost::json::object{ { "label", camera.name }, { "type", "radio" }, { "context", VIEW_CONTEXT },
        { "command", "camera::scene" }, { "data", data } };
}

/**
 **/
void Area::checkCamera(v3d::ui::component::menu::Menu* menu, std::int64_t shown) {
    if (menu == nullptr) {
        return;
    }
    const std::string id = std::to_string(shown);
    for (std::size_t index = 0; index < menu->count(); index++) {
        const boost::shared_ptr<v3d::ui::component::menu::Item>& item = (*menu)[index];
        if (!item || item->event().name() != "camera::scene") {
            continue;
        }
        // the data is the camera's id, followed by the area's after an "@" in an area's menu
        const boost::optional<v3d::event::EventData> data = item->event().data();
        const std::string named = data && std::holds_alternative<std::string>(*data) ? std::get<std::string>(*data) : std::string();
        item->checked(shown != 0 && named.substr(0, named.find('@')) == id);
    }
}

/**
 **/
void Area::restoreKey(const std::string& key) {
    restoreKey_ = key;
}

/**
 **/
void Area::place(const view::AreaLayout::Area& area, const Placing& placing) const {
    const boost::shared_ptr<v3d::ui::Container> container = ui_->container(name(area.id));
    if (!container) {
        return;
    }
    const float height = placing.height;
    const glm::vec4& region = area.region;
    const glm::vec4& content = area.content;
    const bool roomy = content.z > height && content.w > height;
    container->visible(roomy);
    if (!roomy) {
        return;
    }
    const float strip = content.y - region.y;
    const bool tabbed = strip > 0.0F;
    // a maximised area's hint follows the titles of its header, or of its strip's +
    const std::string hint = placing.maximised && !restoreKey_.empty() ? restoreKey_ + " restores the areas" : std::string();

    const boost::shared_ptr<v3d::ui::component::menu::Bar> header =
        boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(container->get("header"));
    if (header) {
        placeHeader(header.get(), area, placing, tabbed ? std::string() : hint);
    }

    float maximiseLeft = content.x + content.z - height;
    if (tabbed) {
        maximiseLeft = region.x + region.z - height;
        placeStrip(*container, area, height, hint);
    }
    if (const boost::shared_ptr<v3d::ui::Component> maximise = container->get("maximise")) {
        box(maximise.get(), glm::vec4(maximiseLeft, tabbed ? region.y : content.y, height, tabbed ? strip : height));
    }
}

/**
 **/
void Area::placeHeader(v3d::ui::component::menu::Bar* bar, const view::AreaLayout::Area& area, const Placing& placing,
    const std::string& note) {
    const glm::vec4& content = area.content;
    const bool tabbed = content.y > area.region.y;
    // a tabbed area's maximise is on its strip, so its header runs the area's width
    box(bar, glm::vec4(content.x, content.y, content.z - (tabbed ? 0.0F : placing.height), placing.height));
    if (!tabbed) {
        check(bar, marks(area.editor()));
    }
    if (placing.view != nullptr) {
        check(bar, marks(*placing.view));
        check(bar, marks(placing.mask));
        // the camera's menu names the active view. It follows the area's menu, which a tabbed
        // area has on its strip instead
        const std::size_t camera = tabbed ? 0 : 1;
        if (bar->count() > camera && bar->menu(camera)) {
            bar->menu(camera)->emphasis(placing.active);
            checkCamera(bar->menu(camera).get(), placing.view->looking());
        }
    }
    bar->note(note);
}

/**
 **/
void Area::placeStrip(const v3d::ui::Container& container, const view::AreaLayout::Area& area, float height, const std::string& hint) {
    const glm::vec4& region = area.region;
    const float strip = area.content.y - region.y;
    if (const boost::shared_ptr<v3d::ui::Component> recess = container.get("strip")) {
        box(recess.get(), glm::vec4(region.x, region.y, region.z, strip));
    }
    // the menu's title ends where it was last drawn. Until then it is taken to be two headers wide
    float menuEnd = region.x + (height * 2.0F);
    const boost::shared_ptr<v3d::ui::component::menu::Bar> menu =
        boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(container.get("strip-menu"));
    if (menu) {
        box(menu.get(), glm::vec4(region.x, region.y, height * 2.0F, strip));
        check(menu.get(), marks(area.editor()));
        if (menu->count() > 0 && ended(*menu) > region.x) {
            menuEnd = ended(*menu);
        }
    }
    // the tabs follow the menu and leave room for the + and maximise
    float tabsEnd = menuEnd;
    if (const boost::shared_ptr<v3d::ui::Component> tabs = container.get("tabs")) {
        box(tabs.get(), glm::vec4(menuEnd, region.y, std::max(0.0F, region.x + region.z - menuEnd - (height * 2.0F)), strip));
        for (std::size_t index = 0; index < area.editors.size(); index++) {
            const boost::shared_ptr<v3d::ui::component::Button> tab = boost::dynamic_pointer_cast<v3d::ui::component::Button>(
                container.get("tab-" + std::to_string(area.editors[index].id)));
            if (tab) {
                tab->checked(index == area.chosen);
                tabsEnd = std::max(tabsEnd, tab->position().x + tab->size().x);
            }
        }
    }
    const boost::shared_ptr<v3d::ui::component::menu::Bar> add = boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(container.get("add"));
    if (add) {
        // the + takes what is left of the strip before maximise, which its note may run into
        box(add.get(), glm::vec4(tabsEnd, region.y, std::max(height, region.x + region.z - height - tabsEnd), strip));
        add->note(hint);
    }
}

/**
 **/
float Area::ended(const v3d::ui::component::menu::Bar& bar) {
    const v3d::type::geometry::Bound2D title = bar.bound(bar.count() - 1);
    return title.position().x + title.size().x;
}

/**
 **/
std::optional<std::size_t> Area::tabAt(const glm::vec2& point) const {
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

};  // namespace v3d::editor::control::chrome
