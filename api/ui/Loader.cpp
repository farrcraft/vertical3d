/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Loader.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "Component.h"
#include "Container.h"
#include "Style.h"
#include "component/Bar.h"
#include "component/Box.h"
#include "component/Button.h"
#include "component/CheckBox.h"
#include "component/HorizontalBox.h"
#include "component/Icon.h"
#include "component/Label.h"
#include "component/Panel.h"
#include "component/RadioButton.h"
#include "component/Scrollbar.h"
#include "component/SelectList.h"
#include "component/TabBar.h"
#include "component/TabPage.h"
#include "component/Toolbar.h"
#include "component/Type.h"
#include "component/VerticalBox.h"
#include "component/menu/Menu.h"
#include "component/menu/MenuBar.h"
#include "component/menu/MenuItem.h"
#include "style/Button.h"
#include "style/Property.h"
#include "style/Theme.h"
#include "style/property/Color.h"
#include "style/property/Font.h"
#include "style/property/Image.h"
#include "style/property/Number.h"

#include "../event/Engine.h"
#include "../log/Logger.h"

#include <boost/json/value_to.hpp>
#include <boost/make_shared.hpp>
#include <entt/entt.hpp>

namespace v3d::ui {

namespace {

/**
 * Read a fixed length array of numbers - a position, a size, a colour.
 *
 * @param fallback what to answer with when the field is absent or the wrong length,
 *      which is what lets every one of these be optional
 **/
template <typename T, std::size_t N>
T numbers(const boost::json::object& entry, const std::string& field, const T& fallback) {
    if (!entry.contains(field) || !entry.at(field).is_array()) {
        return fallback;
    }
    const boost::json::array values = entry.at(field).as_array();
    if (values.size() != N) {
        return fallback;
    }
    T result = fallback;
    for (std::size_t index = 0; index < N; index++) {
        result[static_cast<int>(index)] = static_cast<float>(boost::json::value_to<double>(values[index]));
    }
    return result;
}

/**
 * Read one length: a number is pixels, and a string ending in % is a fraction of the
 * parent. Anything else is left as it was, which is Auto for a component naming nothing.
 **/
Length length(const boost::json::value& value, const Length& fallback) {
    if (value.is_string()) {
        const std::string text(value.as_string().c_str());
        if (text.size() > 1 && text.back() == '%') {
            try {
                return Length(std::stof(text.substr(0, text.size() - 1)), Length::Unit::Percent);
            } catch (const std::exception&) {
                return fallback;
            }
        }
        return fallback;
    }
    if (value.is_number()) {
        return Length(static_cast<float>(boost::json::value_to<double>(value)), Length::Unit::Pixels);
    }
    return fallback;
}

/**
 * Read a pair of lengths out of a two element array - a position or a size.
 **/
void lengths(const boost::json::object& entry, const std::string& field, Length* first, Length* second) {
    if (!entry.contains(field) || !entry.at(field).is_array()) {
        return;
    }
    const boost::json::array values = entry.at(field).as_array();
    if (values.size() != 2) {
        return;
    }
    *first = length(values[0], *first);
    *second = length(values[1], *second);
}

/**
 * The alignment names a style property may carry. An unrecognised one is no
 * alignment rather than an error, which is the only sensible reading of a hint.
 **/
style::Property::Alignment alignment(const boost::json::object& entry) {
    if (!entry.contains("align")) {
        return style::Property::NULL_ALIGNMENT;
    }
    const std::string name = boost::json::value_to<std::string>(entry.at("align"));
    if (name == "top") {
        return style::Property::TOP;
    }
    if (name == "bottom") {
        return style::Property::BOTTOM;
    }
    if (name == "left") {
        return style::Property::LEFT;
    }
    if (name == "right") {
        return style::Property::RIGHT;
    }
    if (name == "top-left") {
        return style::Property::TOP_LEFT;
    }
    if (name == "bottom-left") {
        return style::Property::BOTTOM_LEFT;
    }
    if (name == "top-right") {
        return style::Property::TOP_RIGHT;
    }
    if (name == "bottom-right") {
        return style::Property::BOTTOM_RIGHT;
    }
    if (name == "center") {
        return style::Property::CENTER;
    }
    return style::Property::NULL_ALIGNMENT;
}

/**
 * @return whether the entry carries the field as a boolean, and what it holds
 **/
bool flag(const boost::json::object& entry, const std::string& field, bool fallback) {
    if (!entry.contains(field) || !entry.at(field).is_bool()) {
        return fallback;
    }
    return boost::json::value_to<bool>(entry.at(field));
}

};  // namespace

Loader::Loader(const boost::shared_ptr<v3d::event::Engine>& eventEngine,
    const boost::shared_ptr<entt::dispatcher>& dispatcher,
    const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger),
    eventEngine_(eventEngine),
    dispatcher_(dispatcher) {
}

bool Loader::load(const boost::json::object& doc) {
    if (!loadThemes(doc)) {
        return false;
    }

    auto const containersSection = doc.at("containers");
    if (!containersSection.is_array()) {
        logger_->get()->error("Missing containers in config");
        return false;
    }
    auto const containers = containersSection.as_array();
    for (const auto* it = containers.begin(); it != containers.end(); ++it) {
        if (!it->is_object()) {
            logger_->get()->error("Unrecognized containers config");
            return false;
        }
        if (!loadContainer(it->as_object())) {
            return false;
        }
    }
    return true;
}

std::vector<boost::shared_ptr<Container>>& Loader::containers() noexcept {
    return containers_;
}

std::vector<boost::shared_ptr<style::Theme>>& Loader::themes() noexcept {
    return themes_;
}

const std::string& Loader::active() const noexcept {
    return active_;
}

bool Loader::loadThemes(const boost::json::object& doc) {
    auto const themesSection = doc.at("themes");
    if (!themesSection.is_array()) {
        logger_->get()->error("Missing themes in config");
        return false;
    }
    auto const themes = themesSection.as_array();
    for (const auto* it = themes.begin(); it != themes.end(); ++it) {
        if (!it->is_object()) {
            logger_->get()->error("Unrecognized theme config");
            return false;
        }
        if (!loadTheme(it->as_object())) {
            return false;
        }
    }

    if (doc.contains("theme")) {
        active_ = boost::json::value_to<std::string>(doc.at("theme"));
        // named here and resolved by whoever takes the themes, so that a document naming a
        // theme it does not carry is refused rather than half loaded
        const bool known = std::any_of(themes_.begin(), themes_.end(),
            [this](const boost::shared_ptr<style::Theme>& theme) { return theme->name() == active_; });
        if (!known) {
            logger_->get()->error("Unknown active theme [{}] in config", active_);
            return false;
        }
    }
    return true;
}

bool Loader::loadTheme(const boost::json::object& entry) {
    std::string themeName = boost::json::value_to<std::string>(entry.at("name"));
    boost::shared_ptr<style::Theme> theme = boost::make_shared<style::Theme>(themeName);

    // a theme with no styles in it is legal and draws in the defaults, which is what
    // every ui config in the tree was before the styles could be read
    if (entry.contains("styles")) {
        if (!entry.at("styles").is_array()) {
            logger_->get()->error("Unrecognized styles config in theme [{}]", themeName);
            return false;
        }
        auto const styles = entry.at("styles").as_array();
        for (const auto* styleIterator = styles.begin(); styleIterator != styles.end(); ++styleIterator) {
            if (!styleIterator->is_object()) {
                logger_->get()->error("Unrecognized style config in theme [{}]", themeName);
                return false;
            }
            if (!loadStyle(styleIterator->as_object(), theme)) {
                return false;
            }
        }
    }

    themes_.push_back(theme);
    return true;
}

boost::shared_ptr<Component> Loader::buildComponent(const std::string& componentType, const boost::json::object& entry) {
    boost::shared_ptr<Component> component;
    if (componentType == "menu") {
        boost::shared_ptr<component::Menu> menu = loadMenu(entry);
        if (!menu) {
            return nullptr;
        }
        // this is the menu the app navigates, so it starts as its own active level
        menu->level(menu);
        component = menu;
    } else if (componentType == "menubar") {
        component = loadMenuBar(entry);
    } else if (componentType == "toolbar") {
        component = loadToolbar(entry);
    } else if (componentType == "button") {
        component = loadButton(entry);
    } else if (componentType == "label") {
        component = loadLabel(entry);
    } else if (componentType == "icon") {
        component = loadIcon(entry);
    } else if (componentType == "panel") {
        component = loadPanel(entry);
    } else if (componentType == "bar") {
        component = loadBar(entry);
    } else if (componentType == "scrollbar") {
        component = loadScrollbar(entry);
    } else if (componentType == "list") {
        component = loadSelectList(entry);
    } else if (componentType == "tabs") {
        component = boost::make_shared<component::TabBar>();
    } else if (componentType == "tab") {
        boost::shared_ptr<component::TabPage> page = boost::make_shared<component::TabPage>();
        if (entry.contains("label")) {
            page->label(boost::json::value_to<std::string>(entry.at("label")));
        }
        component = page;
    } else if (componentType == "checkbox") {
        boost::shared_ptr<component::CheckBox> box = boost::make_shared<component::CheckBox>();
        loadCheckBox(entry, box);
        component = box;
    } else if (componentType == "radio") {
        boost::shared_ptr<component::RadioButton> radio = boost::make_shared<component::RadioButton>();
        loadCheckBox(entry, radio);
        if (entry.contains("group")) {
            radio->group(boost::json::value_to<std::string>(entry.at("group")));
        }
        component = radio;
    } else if (componentType == "vbox" || componentType == "hbox") {
        boost::shared_ptr<component::Box> box = componentType == "vbox"
            ? boost::static_pointer_cast<component::Box>(boost::make_shared<component::VerticalBox>())
            : boost::static_pointer_cast<component::Box>(boost::make_shared<component::HorizontalBox>());
        loadBox(entry, box);
        component = box;
    } else {
        logger_->get()->error("Unrecognized ui component type [{}]", componentType);
    }
    return component;
}

boost::shared_ptr<Component> Loader::loadComponent(const boost::json::object& entry) {
    const std::string componentType = boost::json::value_to<std::string>(entry.at("type"));
    const std::string componentName = boost::json::value_to<std::string>(entry.at("name"));

    boost::shared_ptr<Component> component = buildComponent(componentType, entry);
    if (!component) {
        return nullptr;
    }

    component->name(componentName);
    // a menu and a menu bar are placed entirely by the renderer, so reading a box onto one
    // would be read and then written over
    if (componentType != "menu" && componentType != "menubar") {
        loadAttributes(entry, component);
    }
    if (!loadChildren(entry, component)) {
        return nullptr;
    }
    // which tab is up is a place in the pages, so it can only be read once the pages the
    // children array named are there
    if (componentType == "tabs" && entry.contains("selected")) {
        boost::static_pointer_cast<component::TabBar>(component)->selected(
            boost::json::value_to<int>(entry.at("selected")));
    }
    return component;
}

bool Loader::loadChildren(const boost::json::object& entry, const boost::shared_ptr<Component>& component) {
    if (!entry.contains("children")) {
        return true;
    }
    auto const section = entry.at("children");
    if (!section.is_array()) {
        logger_->get()->error("The children of [{}] are not an array", std::string(component->name()));
        return false;
    }
    auto const children = section.as_array();
    for (const auto* it = children.begin(); it != children.end(); ++it) {
        if (!it->is_object()) {
            logger_->get()->error("Unrecognized component config");
            return false;
        }
        const boost::shared_ptr<Component> child = loadComponent(it->as_object());
        if (!child) {
            return false;
        }
        component->add(child);
    }
    return true;
}

bool Loader::loadContainer(const boost::json::object& entry) {
    std::string containerName = boost::json::value_to<std::string>(entry.at("name"));
    bool visible = boost::json::value_to<bool>(entry.at("visible"));
    boost::shared_ptr<Container> container = boost::make_shared<Container>(containerName, visible);
    containers_.push_back(container);

    // read components in this container
    auto const componentsSection = entry.at("components");
    if (!componentsSection.is_array()) {
        logger_->get()->error("Missing components in config");
        return false;
    }
    auto const components = componentsSection.as_array();
    for (const auto* it = components.begin(); it != components.end(); ++it) {
        if (!it->is_object()) {
            logger_->get()->error("Unrecognized component config");
            return false;
        }
        const boost::shared_ptr<Component> component = loadComponent(it->as_object());
        if (!component) {
            return false;
        }
        container->add(component);
    }
    return true;
}

/**
 **/
bool Loader::loadStyle(const boost::json::object& entry, const boost::shared_ptr<style::Theme>& theme) {
    if (!entry.contains("class") || !entry.contains("name")) {
        logger_->get()->error("A style needs both a class and a name");
        return false;
    }
    const std::string className = boost::json::value_to<std::string>(entry.at("class"));
    const std::string styleName = boost::json::value_to<std::string>(entry.at("name"));

    boost::shared_ptr<Style> target;
    if (className == "button") {
        // a button is drawn differently in each of its states, so its styles are told
        // apart by the state as well as by the name
        component::Button::ButtonState state = component::Button::STATE_NORMAL;
        const std::string stateName = entry.contains("state")
            ? boost::json::value_to<std::string>(entry.at("state")) : std::string("normal");
        if (stateName == "hover") {
            state = component::Button::STATE_HOVER;
        } else if (stateName == "press") {
            state = component::Button::STATE_PRESS;
        } else if (stateName == "inactive") {
            state = component::Button::STATE_INACTIVE;
        } else if (stateName != "normal") {
            logger_->get()->error("A button style has no state [{}]", stateName);
            return false;
        }
        target = boost::make_shared<style::Button>(styleName, state);
    } else {
        target = boost::make_shared<Style>(styleName, className);
    }

    if (!loadProperties(entry, target)) {
        return false;
    }
    theme->addStyle(target);
    return true;
}

bool Loader::loadProperties(const boost::json::object& entry, const boost::shared_ptr<Style>& target) {
    // the four arrays are the four property classes a style is asked for by, so what a
    // property is read as is where it was written rather than a field it carries
    static const char* const classes[] = { "colors", "numbers", "fonts", "images" };

    for (const char* const section : classes) {
        if (!entry.contains(section)) {
            continue;
        }
        if (!entry.at(section).is_array()) {
            logger_->get()->error("Unrecognized {} in a style", section);
            return false;
        }
        auto const properties = entry.at(section).as_array();
        for (const auto* it = properties.begin(); it != properties.end(); ++it) {
            if (!it->is_object() || !it->as_object().contains("name")) {
                logger_->get()->error("A style property needs a name");
                return false;
            }
            auto const property = it->as_object();
            const std::string name = boost::json::value_to<std::string>(property.at("name"));

            std::string propertyClass;
            boost::shared_ptr<style::Property> loaded = loadProperty(section, property, name, &propertyClass);
            if (!loaded) {
                return false;
            }
            loaded->align(alignment(property));
            target->addProperty(loaded, propertyClass);
        }
    }
    return true;
}

/**
 **/
boost::shared_ptr<style::Property> Loader::loadProperty(const std::string& section,
    const boost::json::object& property, const std::string& name, std::string* propertyClass) {
    if (section == "colors") {
        *propertyClass = "color";
        return boost::make_shared<style::property::Color>(name,
            numbers<glm::vec4, 4>(property, "value", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)));
    }
    if (section == "numbers") {
        if (!property.contains("value") || !property.at("value").is_number()) {
            logger_->get()->error("The style number [{}] has no value", name);
            return nullptr;
        }
        *propertyClass = "number";
        return boost::make_shared<style::property::Number>(name,
            static_cast<float>(boost::json::value_to<double>(property.at("value"))));
    }
    if (section == "fonts") {
        boost::shared_ptr<style::property::Font> font = boost::make_shared<style::property::Font>(name,
            property.contains("source") ? boost::json::value_to<std::string>(property.at("source")) : std::string());
        if (property.contains("face")) {
            font->face(boost::json::value_to<std::string>(property.at("face")));
        }
        if (property.contains("size")) {
            font->size(static_cast<unsigned int>(boost::json::value_to<double>(property.at("size"))));
        }
        font->bold(flag(property, "bold", false));
        font->italics(flag(property, "italics", false));
        *propertyClass = "font";
        return font;
    }
    if (!property.contains("source")) {
        logger_->get()->error("The style image [{}] names no source", name);
        return nullptr;
    }
    *propertyClass = "image";
    return boost::make_shared<style::property::Image>(name,
        boost::json::value_to<std::string>(property.at("source")));
}

/**
 **/
void Loader::loadAttributes(const boost::json::object& entry, const boost::shared_ptr<Component>& component) {
    loadLayout(entry, &component->layout());
    if (entry.contains("style")) {
        component->style(boost::json::value_to<std::string>(entry.at("style")));
    }
    component->visible(flag(entry, "visible", component->visible()));
    component->pickable(flag(entry, "pickable", component->pickable()));
    component->clip(flag(entry, "clip", component->clip()));
    if (entry.contains("depth")) {
        component->depth(boost::json::value_to<unsigned int>(entry.at("depth")));
    }
}

/**
 **/
void Loader::loadLayout(const boost::json::object& entry, Layout* layout) {
    lengths(entry, "position", &layout->x, &layout->y);
    lengths(entry, "size", &layout->width, &layout->height);
    if (!entry.contains("anchor")) {
        return;
    }
    const std::string name = boost::json::value_to<std::string>(entry.at("anchor"));
    if (name == "top-right") {
        layout->anchor = Layout::Anchor::TopRight;
    } else if (name == "bottom-left") {
        layout->anchor = Layout::Anchor::BottomLeft;
    } else if (name == "bottom-right") {
        layout->anchor = Layout::Anchor::BottomRight;
    } else if (name == "centre" || name == "center") {
        layout->anchor = Layout::Anchor::Centre;
    } else if (name != "top-left") {
        logger_->get()->error("A component has no anchor [{}]", name);
    }
}

/**
 **/
boost::shared_ptr<component::Button> Loader::loadButton(const boost::json::object& entry) {
    boost::shared_ptr<component::Button> button = boost::make_shared<component::Button>();
    if (entry.contains("label")) {
        button->label(boost::json::value_to<std::string>(entry.at("label")));
    }
    if (entry.contains("icon")) {
        button->icon(boost::json::value_to<std::string>(entry.at("icon")));
    }
    button->toggle(flag(entry, "toggle", false));
    const v3d::event::Event command = loadCommand(entry);
    if (command.context()) {
        button->event(command);
    }
    return button;
}

/**
 **/
boost::shared_ptr<component::Panel> Loader::loadPanel(const boost::json::object& entry) {
    static_cast<void>(entry);
    // everything a panel is drawn with is its style's, per ADR-0020, so there is nothing
    // of its own to read
    return boost::make_shared<component::Panel>();
}

/**
 **/
boost::shared_ptr<component::Bar> Loader::loadBar(const boost::json::object& entry) {
    boost::shared_ptr<component::Bar> bar = boost::make_shared<component::Bar>();
    if (entry.contains("fraction")) {
        bar->fraction(static_cast<float>(boost::json::value_to<double>(entry.at("fraction"))));
    }
    if (entry.contains("direction")
        && boost::json::value_to<std::string>(entry.at("direction")) == "vertical") {
        bar->direction(component::Bar::Direction::Vertical);
    }
    return bar;
}

/**
 **/
boost::shared_ptr<component::Scrollbar> Loader::loadScrollbar(const boost::json::object& entry) {
    boost::shared_ptr<component::Scrollbar> bar = boost::make_shared<component::Scrollbar>();
    if (entry.contains("direction")
        && boost::json::value_to<std::string>(entry.at("direction")) == "horizontal") {
        bar->direction(component::Scrollbar::Direction::Horizontal);
    }
    // what there is to scroll is written by whatever fills the thing being scrolled, so a
    // range in the config is a starting one rather than the truth
    if (entry.contains("content") && entry.contains("page")) {
        bar->range(static_cast<float>(boost::json::value_to<double>(entry.at("content"))),
            static_cast<float>(boost::json::value_to<double>(entry.at("page"))));
    }
    if (entry.contains("offset")) {
        bar->offset(static_cast<float>(boost::json::value_to<double>(entry.at("offset"))));
    }
    return bar;
}

/**
 **/
boost::shared_ptr<component::SelectList> Loader::loadSelectList(const boost::json::object& entry) {
    boost::shared_ptr<component::SelectList> list = boost::make_shared<component::SelectList>();
    if (entry.contains("items")) {
        const auto& section = entry.at("items");
        if (!section.is_array()) {
            logger_->get()->error("The items of a list are not an array");
            return nullptr;
        }
        std::vector<std::string> rows;
        for (const auto& item : section.as_array()) {
            rows.push_back(boost::json::value_to<std::string>(item));
        }
        list->items(rows);
    }
    // the rows a list holds are usually its app's rather than its config's, so a config
    // that names none is a list something else fills
    if (entry.contains("selected")) {
        list->selected(boost::json::value_to<int>(entry.at("selected")));
    }
    const v3d::event::Event command = loadCommand(entry);
    if (command.context()) {
        list->event(command);
    }
    return list;
}

/**
 **/
void Loader::loadCheckBox(const boost::json::object& entry, const boost::shared_ptr<component::CheckBox>& box) {
    if (entry.contains("label")) {
        box->label(boost::json::value_to<std::string>(entry.at("label")));
    }
    // a mark in the config is the state the ui starts in; after that it is whatever
    // answers the command that sets one, per ADR-0019
    box->checked(flag(entry, "checked", false));
    const v3d::event::Event command = loadCommand(entry);
    if (command.context()) {
        box->event(command);
    }
}

/**
 **/
void Loader::loadBox(const boost::json::object& entry, const boost::shared_ptr<component::Box>& box) {
    if (entry.contains("spacing")) {
        box->spacing(static_cast<float>(boost::json::value_to<double>(entry.at("spacing"))));
    }
    box->stretch(flag(entry, "stretch", box->stretch()));
}

/**
 **/
boost::shared_ptr<component::Label> Loader::loadLabel(const boost::json::object& entry) {
    boost::shared_ptr<component::Label> label = boost::make_shared<component::Label>();
    if (entry.contains("label")) {
        label->text(boost::json::value_to<std::string>(entry.at("label")));
    }
    return label;
}

/**
 **/
boost::shared_ptr<component::Icon> Loader::loadIcon(const boost::json::object& entry) {
    if (!entry.contains("source")) {
        logger_->get()->error("An icon names no source");
        return nullptr;
    }
    return boost::make_shared<component::Icon>(boost::json::value_to<std::string>(entry.at("source")));
}

/**
 **/
boost::shared_ptr<component::MenuBar> Loader::loadMenuBar(const boost::json::object& entry) {
    boost::shared_ptr<component::MenuBar> bar = boost::make_shared<component::MenuBar>();

    auto const menusSection = entry.at("menus");
    if (!menusSection.is_array()) {
        logger_->get()->error("Missing menus in config");
        return nullptr;
    }
    auto const menus = menusSection.as_array();
    const auto* menuIterator = menus.begin();
    for (; menuIterator != menus.end(); ++menuIterator) {
        if (!menuIterator->is_object()) {
            logger_->get()->error("Unrecognized menu config");
            return nullptr;
        }
        auto const menuConfig = menuIterator->as_object();
        std::string label = boost::json::value_to<std::string>(menuConfig.at("label"));
        boost::shared_ptr<component::Menu> menu = loadMenu(menuConfig);
        if (!menu) {
            return nullptr;
        }
        menu->name(label);
        // a bar's menus are dropped rather than navigated, so none of them is a level and
        // none starts with an item active
        menu->active(-1);
        bar->add(label, menu);
    }
    return bar;
}

/**
 **/
boost::shared_ptr<component::Menu> Loader::loadMenu(const boost::json::object& entry) {
    boost::shared_ptr<component::Menu> menu = boost::make_shared<component::Menu>(dispatcher_);

    auto const itemsSection = entry.at("items");
    if (!itemsSection.is_array()) {
        logger_->get()->error("Missing menu items in config");
        return nullptr;
    }
    auto const items = itemsSection.as_array();
    const auto* itemsIterator = items.begin();
    for (; itemsIterator != items.end(); ++itemsIterator) {
        if (!itemsIterator->is_object()) {
            logger_->get()->error("Unrecognized menu item config");
            return nullptr;
        }
        auto const menuItemConfig = itemsIterator->as_object();
        std::string label = boost::json::value_to<std::string>(menuItemConfig.at("label"));
        std::string itemType = boost::json::value_to<std::string>(menuItemConfig.at("type"));

        const v3d::event::Event command = loadCommand(menuItemConfig);

        boost::shared_ptr<component::MenuItem> menuItem = boost::make_shared<component::MenuItem>(component::menu::stringToType(itemType), label);
        // the owning menu has to be set before the submenu below, which reads it to find its parent
        menuItem->menu(menu);
        if (command.context()) {
            menuItem->event(command);
        }

        menu->addItem(menuItem);

        if (menuItem->type() == component::menu::ItemType::Submenu) {
            boost::shared_ptr<component::Menu> submenu = loadMenu(menuItemConfig);
            if (!submenu) {  // submenu(null) would fault setting the parent
                return nullptr;
            }
            menuItem->submenu(submenu);
        }
    }
    menu->active(0);
    return menu;
}

/**
 **/
v3d::event::Event Loader::loadCommand(const boost::json::object& entry) {
    std::string command;
    std::string context;
    if (entry.contains("command")) {
        command = boost::json::value_to<std::string>(entry.at("command"));
    }
    if (entry.contains("context")) {
        context = boost::json::value_to<std::string>(entry.at("context"));
    }
    if (context.empty() || command.empty()) {
        return v3d::event::Event();
    }
    return v3d::event::Event(command, eventEngine_->resolveContext(context));
}

/**
 **/
boost::shared_ptr<component::Toolbar> Loader::loadToolbar(const boost::json::object& entry) {
    std::string edgeName = "top";
    if (entry.contains("edge")) {
        edgeName = boost::json::value_to<std::string>(entry.at("edge"));
    }
    component::Toolbar::Edge edge = component::Toolbar::Edge::Top;
    if (edgeName == "left") {
        edge = component::Toolbar::Edge::Left;
    } else if (edgeName != "top") {
        logger_->get()->error("A toolbar runs along the top or the left edge, not [{}]", edgeName);
        return nullptr;
    }

    boost::shared_ptr<component::Toolbar> bar = boost::make_shared<component::Toolbar>(dispatcher_, edge);

    auto const buttonsSection = entry.at("buttons");
    if (!buttonsSection.is_array()) {
        logger_->get()->error("Missing toolbar buttons in config");
        return nullptr;
    }
    auto const buttons = buttonsSection.as_array();
    const auto* buttonIterator = buttons.begin();
    for (; buttonIterator != buttons.end(); ++buttonIterator) {
        if (!buttonIterator->is_object()) {
            logger_->get()->error("Unrecognized toolbar button config");
            return nullptr;
        }
        // a button in a strip is read the same way a button in a container is
        bar->add(loadButton(buttonIterator->as_object()));
    }
    return bar;
}

};  // namespace v3d::ui
