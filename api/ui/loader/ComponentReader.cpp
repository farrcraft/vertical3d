/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ComponentReader.h"

#include <api/asset/Json.h>
#include <api/event/Event.h>
#include <api/event/Router.h>
#include <api/log/Logger.h>
#include <api/ui/Component.h>
#include <api/ui/Container.h>
#include <api/ui/Layout.h>
#include <api/ui/Length.h>
#include <api/ui/component/Bar.h>
#include <api/ui/component/Box.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/CheckBox.h>
#include <api/ui/component/HorizontalBox.h>
#include <api/ui/component/Icon.h>
#include <api/ui/component/Label.h>
#include <api/ui/component/Panel.h>
#include <api/ui/component/RadioButton.h>
#include <api/ui/component/Scrollbar.h>
#include <api/ui/component/SelectList.h>
#include <api/ui/component/Slider.h>
#include <api/ui/component/TabBar.h>
#include <api/ui/component/TabPage.h>
#include <api/ui/component/TextBox.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/component/Type.h>
#include <api/ui/component/VerticalBox.h>
#include <api/ui/component/menu/Bar.h>
#include <api/ui/component/menu/Item.h>
#include <api/ui/component/menu/Menu.h>
#include <api/ui/component/menu/Type.h>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <vector>

#include "Members.h"

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/value.hpp>
#include <boost/make_shared.hpp>
#include <entt/entt.hpp>

namespace v3d::ui::loader {

namespace {

/**
 * Read one length: a number is pixels, and a string ending in % is a fraction of the parent.
 * @return the length, or nothing when the value is neither
 **/
std::optional<Length> length(const boost::json::value& value) {
    if (value.is_number()) {
        return Length(static_cast<float>(value.to_number<double>()), Length::Unit::Pixels);
    }
    if (!value.is_string()) {
        return std::nullopt;
    }
    const std::string text(value.as_string().c_str());
    if (text.size() < 2 || text.back() != '%') {
        return std::nullopt;
    }
    try {
        std::size_t read = 0;
        const float fraction = std::stof(text.substr(0, text.size() - 1), &read);
        if (read != text.size() - 1) {
            return std::nullopt;
        }
        return Length(fraction, Length::Unit::Percent);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

};  // namespace

ComponentReader::ComponentReader(const boost::shared_ptr<v3d::event::Router>& router,
    const boost::shared_ptr<entt::dispatcher>& dispatcher,
    const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger),
    router_(router),
    dispatcher_(dispatcher) {
}

bool ComponentReader::read(const boost::json::object& doc) {
    const boost::json::array* containers = requiredArray(*logger_, doc, "containers", "A ui document");
    if (containers == nullptr) {
        return false;
    }
    return std::ranges::all_of(*containers, [this](const boost::json::value& item) {
        const boost::json::object* container = requiredObject(*logger_, item, "The containers");
        return container != nullptr && readContainer(*container);
    });
}

std::vector<boost::shared_ptr<Container>>& ComponentReader::containers() noexcept {
    return containers_;
}

bool ComponentReader::readContainer(const boost::json::object& entry) {
    const std::optional<std::string> named = requiredString(*logger_, entry, "name", "A container");
    if (!named) {
        return false;
    }
    const std::optional<bool> visible = v3d::asset::readBool(entry, "visible");
    if (!visible) {
        logger_->get()->error("The container [{}] needs visible to be true or false", *named);
        return false;
    }
    const boost::json::array* components = requiredArray(*logger_, entry, "components", "A container");
    if (components == nullptr) {
        return false;
    }
    const boost::shared_ptr<Container> container = boost::make_shared<Container>(*named, *visible);
    containers_.push_back(container);

    return std::ranges::all_of(*components, [this, &container](const boost::json::value& item) {
        const boost::json::object* object = requiredObject(*logger_, item, "A container's components");
        const boost::shared_ptr<Component> component = object == nullptr ? nullptr : readComponent(*object);
        if (!component) {
            return false;
        }
        container->add(component);
        return true;
    });
}

boost::shared_ptr<Component> ComponentReader::buildComponent(const std::string& componentType,
    const boost::json::object& entry) {
    // the document's vocabulary is component::name()'s, so this switches on what a type is
    // rather than on what it was spelled. An exhaustive switch, so a component added to the
    // enum fails the build here until it is given a way to be built
    switch (component::parse(componentType)) {
        case component::Type::Menu: {
            boost::shared_ptr<component::menu::Menu> menu = readMenu(entry);
            if (!menu) {
                return nullptr;
            }
            // this is the menu the app navigates, so it starts as its own active level
            menu->level(menu);
            return menu;
        }
        case component::Type::MenuBar:
            return readMenuBar(entry);
        case component::Type::Toolbar:
            return readToolbar(entry);
        case component::Type::Button:
            return readButton(entry);
        case component::Type::Label:
            return readLabel(entry);
        case component::Type::Icon:
            return readIcon(entry);
        case component::Type::Panel:
            // everything a panel is drawn with comes from its style, so there is nothing of
            // its own to read
            return boost::make_shared<component::Panel>();
        case component::Type::Bar:
            return readBar(entry);
        case component::Type::Scrollbar:
            return readScrollbar(entry);
        case component::Type::Slider:
            return readSlider(entry);
        case component::Type::SelectList:
            return readSelectList(entry);
        case component::Type::TextBox:
            return readTextBox(entry);
        case component::Type::TabBar:
            return boost::make_shared<component::TabBar>();
        case component::Type::TabPage:
            return readTabPage(entry);
        case component::Type::CheckBox: {
            boost::shared_ptr<component::CheckBox> box = boost::make_shared<component::CheckBox>();
            readCheckBox(entry, box);
            return box;
        }
        case component::Type::RadioButton:
            return readRadioButton(entry);
        case component::Type::HorizontalBox:
        case component::Type::VerticalBox:
            return readFlowBox(componentType, entry);
        case component::Type::MenuItem:
            // built by the menu that holds it, not named as a component of its own, so parse()
            // never returns this, and a document that names it is handled as Undefined
        case component::Type::Undefined:
            break;
    }
    logger_->get()->error("Unrecognized ui component type [{}]", componentType);
    return nullptr;
}

boost::shared_ptr<Component> ComponentReader::readComponent(const boost::json::object& entry) {
    const std::optional<std::string> typed = requiredString(*logger_, entry, "type", "A component");
    const std::optional<std::string> named = requiredString(*logger_, entry, "name", "A component");
    if (!typed || !named) {
        return nullptr;
    }
    const std::string& componentType = *typed;

    boost::shared_ptr<Component> component = buildComponent(componentType, entry);
    if (!component) {
        return nullptr;
    }

    component->name(*named);
    // the renderer places a menu and a menu bar itself, so a layout read onto one would be
    // ignored. Their other attributes are read as for any component
    const component::Type type = component::parse(componentType);
    if (type != component::Type::Menu && type != component::Type::MenuBar) {
        readLayout(entry, &component->layout());
    }
    readAttributes(entry, component);
    if (!readChildren(entry, component)) {
        return nullptr;
    }
    // the chosen tab is an index into the pages, so it is read only after the children array
    // has added them
    if (type == component::Type::TabBar) {
        if (const std::optional<int> selected = optionalInt(*logger_, entry, "selected", 0)) {
            boost::static_pointer_cast<component::TabBar>(component)->selected(*selected);
        }
    }
    return component;
}

bool ComponentReader::readChildren(const boost::json::object& entry, const boost::shared_ptr<Component>& component) {
    const boost::json::array* children = optionalArray(*logger_, entry, "children");
    if (children == nullptr) {
        return true;
    }
    return std::ranges::all_of(*children, [this, &component](const boost::json::value& item) {
        const boost::json::object* object = requiredObject(*logger_, item, "A component's children");
        const boost::shared_ptr<Component> child = object == nullptr ? nullptr : readComponent(*object);
        if (!child) {
            return false;
        }
        component->add(child);
        return true;
    });
}

void ComponentReader::readAttributes(const boost::json::object& entry, const boost::shared_ptr<Component>& component) {
    if (const std::optional<std::string> value = optionalString(*logger_, entry, "style")) {
        component->style(*value);
    }
    component->visible(optionalFlag(*logger_, entry, "visible", component->visible()));
    component->enabled(optionalFlag(*logger_, entry, "enabled", component->enabled()));
    component->pickable(optionalFlag(*logger_, entry, "pickable", component->pickable()));
    component->focusable(optionalFlag(*logger_, entry, "focusable", component->focusable()));
    component->clip(optionalFlag(*logger_, entry, "clip", component->clip()));
    if (const std::optional<int> depth = optionalInt(*logger_, entry, "depth", 0)) {
        component->depth(static_cast<unsigned int>(*depth));
    }
}

void ComponentReader::readLayout(const boost::json::object& entry, Layout* layout) {
    readLengths(entry, "position", &layout->x, &layout->y);
    readLengths(entry, "size", &layout->width, &layout->height);
    layout->anchor = optionalChoice(*logger_, entry, "anchor", {
        { "top-left", Layout::Anchor::TopLeft },
        { "top-right", Layout::Anchor::TopRight },
        { "bottom-left", Layout::Anchor::BottomLeft },
        { "bottom-right", Layout::Anchor::BottomRight },
        { "centre", Layout::Anchor::Centre },
        { "center", Layout::Anchor::Centre } }, layout->anchor);
}

void ComponentReader::readLengths(const boost::json::object& entry, const std::string& key, Length* first,
    Length* second) {
    const boost::json::array* values = optionalArray(*logger_, entry, key);
    if (values == nullptr) {
        return;
    }
    if (values->size() != 2) {
        logger_->get()->warn("The ui member [{}] is not two lengths, so it is ignored", key);
        return;
    }
    Length* const into[] = { first, second };
    for (std::size_t index = 0; index < 2; index++) {
        const std::optional<Length> read = length((*values)[index]);
        if (read) {
            *into[index] = *read;
        } else {
            logger_->get()->warn("A length in the ui member [{}] is not pixels or a percentage, so it is ignored", key);
        }
    }
}

boost::shared_ptr<component::Button> ComponentReader::readButton(const boost::json::object& entry) {
    boost::shared_ptr<component::Button> button = boost::make_shared<component::Button>();
    if (const std::optional<std::string> value = optionalString(*logger_, entry, "label")) {
        button->label(*value);
    }
    if (const std::optional<std::string> value = optionalString(*logger_, entry, "icon")) {
        button->icon(*value);
    }
    button->toggle(optionalFlag(*logger_, entry, "toggle", false));
    const v3d::event::Event command = readCommand(entry);
    if (command.context()) {
        button->event(command);
    }
    return button;
}

boost::shared_ptr<component::Bar> ComponentReader::readBar(const boost::json::object& entry) {
    boost::shared_ptr<component::Bar> bar = boost::make_shared<component::Bar>();
    if (const std::optional<float> fraction = optionalNumber(*logger_, entry, "fraction")) {
        bar->fraction(*fraction);
    }
    bar->direction(optionalChoice(*logger_, entry, "direction", {
        { "horizontal", component::Bar::Direction::Horizontal },
        { "vertical", component::Bar::Direction::Vertical } }, bar->direction()));
    return bar;
}

boost::shared_ptr<component::Slider> ComponentReader::readSlider(const boost::json::object& entry) {
    boost::shared_ptr<component::Slider> slider = boost::make_shared<component::Slider>();
    slider->range({.minimum = optionalNumber(*logger_, entry, "minimum").value_or(0.0F),
        .maximum = optionalNumber(*logger_, entry, "maximum").value_or(1.0F),
        .step = optionalNumber(*logger_, entry, "step").value_or(0.0F)});
    slider->value(optionalNumber(*logger_, entry, "value").value_or(slider->minimum()));
    const v3d::event::Event command = readCommand(entry);
    if (command.context()) {
        slider->event(command);
    }
    return slider;
}

boost::shared_ptr<component::Scrollbar> ComponentReader::readScrollbar(const boost::json::object& entry) {
    boost::shared_ptr<component::Scrollbar> bar = boost::make_shared<component::Scrollbar>();
    bar->direction(optionalChoice(*logger_, entry, "direction", {
        { "vertical", component::Scrollbar::Direction::Vertical },
        { "horizontal", component::Scrollbar::Direction::Horizontal } }, bar->direction()));
    // whatever fills the scrolled component sets the real range, so a range in the document
    // is only a starting one
    const std::optional<float> content = optionalNumber(*logger_, entry, "content");
    const std::optional<float> page = optionalNumber(*logger_, entry, "page");
    if (content && page) {
        bar->range(*content, *page);
    }
    if (const std::optional<float> offset = optionalNumber(*logger_, entry, "offset")) {
        bar->offset(*offset);
    }
    return bar;
}

boost::shared_ptr<component::SelectList> ComponentReader::readSelectList(const boost::json::object& entry) {
    boost::shared_ptr<component::SelectList> list = boost::make_shared<component::SelectList>();
    // the app usually fills a list's rows, so a document may name none
    if (const boost::json::array* items = optionalArray(*logger_, entry, "items")) {
        std::vector<std::string> rows;
        for (const boost::json::value& item : *items) {
            if (!item.is_string()) {
                logger_->get()->error("A list has an item that is not a string");
                return nullptr;
            }
            rows.emplace_back(item.as_string().c_str());
        }
        list->items(rows);
    }
    if (const std::optional<int> selected = optionalInt(*logger_, entry, "selected", -1)) {
        list->selected(*selected);
    }
    const v3d::event::Event command = readCommand(entry);
    if (command.context()) {
        list->event(command);
    }
    return list;
}

boost::shared_ptr<component::TextBox> ComponentReader::readTextBox(const boost::json::object& entry) {
    boost::shared_ptr<component::TextBox> box = boost::make_shared<component::TextBox>();
    if (const std::optional<std::string> value = optionalString(*logger_, entry, "text")) {
        box->text(*value);
    }
    if (const std::optional<std::string> value = optionalString(*logger_, entry, "placeholder")) {
        box->placeholder(*value);
    }
    if (const std::optional<int> limit = optionalInt(*logger_, entry, "limit", 0)) {
        box->limit(static_cast<std::size_t>(*limit));
    }
    const v3d::event::Event command = readCommand(entry);
    if (command.context()) {
        box->event(command);
    }
    return box;
}

boost::shared_ptr<component::TabPage> ComponentReader::readTabPage(const boost::json::object& entry) {
    boost::shared_ptr<component::TabPage> page = boost::make_shared<component::TabPage>();
    if (const std::optional<std::string> value = optionalString(*logger_, entry, "label")) {
        page->label(*value);
    }
    return page;
}

boost::shared_ptr<component::RadioButton> ComponentReader::readRadioButton(const boost::json::object& entry) {
    boost::shared_ptr<component::RadioButton> radio = boost::make_shared<component::RadioButton>();
    readCheckBox(entry, radio);
    if (const std::optional<std::string> value = optionalString(*logger_, entry, "group")) {
        radio->group(*value);
    }
    return radio;
}

boost::shared_ptr<component::Box> ComponentReader::readFlowBox(const std::string& componentType,
    const boost::json::object& entry) {
    boost::shared_ptr<component::Box> box = component::parse(componentType) == component::Type::VerticalBox
        ? boost::static_pointer_cast<component::Box>(boost::make_shared<component::VerticalBox>())
        : boost::static_pointer_cast<component::Box>(boost::make_shared<component::HorizontalBox>());
    readBox(entry, box);
    return box;
}

void ComponentReader::readCheckBox(const boost::json::object& entry, const boost::shared_ptr<component::CheckBox>& box) {
    if (const std::optional<std::string> value = optionalString(*logger_, entry, "label")) {
        box->label(*value);
    }
    // "checked" in the document is the state the ui starts in. After that, whatever handles
    // the command sets it
    box->checked(optionalFlag(*logger_, entry, "checked", false));
    const v3d::event::Event command = readCommand(entry);
    if (command.context()) {
        box->event(command);
    }
}

void ComponentReader::readBox(const boost::json::object& entry, const boost::shared_ptr<component::Box>& box) {
    if (const std::optional<float> spacing = optionalNumber(*logger_, entry, "spacing")) {
        box->spacing(*spacing);
    }
    box->stretch(optionalFlag(*logger_, entry, "stretch", box->stretch()));
    box->wrap(optionalFlag(*logger_, entry, "wrap", box->wrap()));
}

boost::shared_ptr<component::Label> ComponentReader::readLabel(const boost::json::object& entry) {
    boost::shared_ptr<component::Label> label = boost::make_shared<component::Label>();
    if (const std::optional<std::string> value = optionalString(*logger_, entry, "label")) {
        label->text(*value);
    }
    return label;
}

boost::shared_ptr<component::Icon> ComponentReader::readIcon(const boost::json::object& entry) {
    const std::optional<std::string> source = requiredString(*logger_, entry, "source", "An icon");
    if (!source) {
        return nullptr;
    }
    return boost::make_shared<component::Icon>(*source);
}

boost::shared_ptr<component::menu::Bar> ComponentReader::readMenuBar(const boost::json::object& entry) {
    boost::shared_ptr<component::menu::Bar> bar = boost::make_shared<component::menu::Bar>();

    const boost::json::array* menus = requiredArray(*logger_, entry, "menus", "A menu bar");
    if (menus == nullptr) {
        return nullptr;
    }
    for (const boost::json::value& item : *menus) {
        const boost::json::object* menuEntry = requiredObject(*logger_, item, "A menu bar's menus");
        if (menuEntry == nullptr) {
            return nullptr;
        }
        const std::optional<std::string> label = requiredString(*logger_, *menuEntry, "label", "A menu");
        if (!label) {
            return nullptr;
        }
        const boost::shared_ptr<component::menu::Menu> menu = readMenu(*menuEntry);
        if (!menu) {
            return nullptr;
        }
        menu->name(*label);
        // a bar's menus are dropped rather than navigated, so none of them is a level and none
        // starts with an item active
        menu->active(-1);
        bar->add(*label, menu);
    }
    return bar;
}

boost::shared_ptr<component::menu::Menu> ComponentReader::readMenu(const boost::json::object& entry) {
    boost::shared_ptr<component::menu::Menu> menu = boost::make_shared<component::menu::Menu>(dispatcher_);

    const boost::json::array* items = requiredArray(*logger_, entry, "items", "A menu");
    if (items == nullptr) {
        return nullptr;
    }
    for (const boost::json::value& item : *items) {
        const boost::json::object* itemEntry = requiredObject(*logger_, item, "A menu's items");
        if (itemEntry == nullptr) {
            return nullptr;
        }
        const std::optional<std::string> label = requiredString(*logger_, *itemEntry, "label", "A menu item");
        const std::optional<std::string> typeName = requiredString(*logger_, *itemEntry, "type", "A menu item");
        if (!label || !typeName) {
            return nullptr;
        }
        const component::menu::ItemType itemType = component::menu::stringToType(*typeName);
        if (itemType == component::menu::ItemType::Unknown) {
            logger_->get()->error("A menu item has no type [{}]", *typeName);
            return nullptr;
        }

        const v3d::event::Event command = readCommand(*itemEntry);

        const boost::shared_ptr<component::menu::Item> menuItem = boost::make_shared<component::menu::Item>(itemType, *label);
        // the owning menu has to be set before the submenu below, which reads it to find its parent
        menuItem->menu(menu);
        if (command.context()) {
            menuItem->event(command);
        }

        menu->addItem(menuItem);

        if (itemType == component::menu::ItemType::Submenu) {
            const boost::shared_ptr<component::menu::Menu> submenu = readMenu(*itemEntry);
            if (!submenu) {  // a submenu that fails to read fails the whole menu
                return nullptr;
            }
            menuItem->submenu(submenu);
        }
    }
    menu->active(0);
    return menu;
}

v3d::event::Event ComponentReader::readCommand(const boost::json::object& entry) {
    const std::string command = optionalString(*logger_, entry, "command").value_or(std::string());
    const std::string context = optionalString(*logger_, entry, "context").value_or(std::string());
    if (context.empty() || command.empty()) {
        return v3d::event::Event();
    }
    return v3d::event::Event(command, router_->resolveContext(context));
}

boost::shared_ptr<component::Toolbar> ComponentReader::readToolbar(const boost::json::object& entry) {
    const component::Toolbar::Edge edge = optionalChoice(*logger_, entry, "edge", {
        { "top", component::Toolbar::Edge::Top },
        { "left", component::Toolbar::Edge::Left } }, component::Toolbar::Edge::Top);
    boost::shared_ptr<component::Toolbar> bar = boost::make_shared<component::Toolbar>(dispatcher_, edge);

    const boost::json::array* buttons = requiredArray(*logger_, entry, "buttons", "A toolbar");
    if (buttons == nullptr) {
        return nullptr;
    }
    for (const boost::json::value& item : *buttons) {
        const boost::json::object* buttonEntry = requiredObject(*logger_, item, "A toolbar's buttons");
        if (buttonEntry == nullptr) {
            return nullptr;
        }
        // a toolbar button is read the same way as a button in a container, so "enabled",
        // "style", "name" and "visible" on one mean what they mean anywhere else
        const boost::shared_ptr<component::Button> button = readButton(*buttonEntry);
        readAttributes(*buttonEntry, button);
        if (const std::optional<std::string> name = optionalString(*logger_, *buttonEntry, "name")) {
            button->name(*name);
        }
        bar->add(button);
    }
    return bar;
}

};  // namespace v3d::ui::loader
