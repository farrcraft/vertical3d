/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Engine.h"

#include <cstddef>
#include <exception>
#include <string>
#include <vector>

#include "component/HorizontalBox.h"
#include "component/VerticalBox.h"
#include "component/menu/MenuBar.h"
#include "component/menu/MenuItem.h"
#include "style/Button.h"
#include "style/property/Color.h"
#include "style/property/Font.h"
#include "style/property/Image.h"
#include "style/property/Number.h"

#include <boost/make_shared.hpp>

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

Engine::Engine(const boost::shared_ptr<v3d::event::Engine>& eventEngine, const boost::shared_ptr<entt::dispatcher>& dispatcher,
    const boost::shared_ptr<v3d::log::Logger>& logger) :
    eventEngine_(eventEngine), dispatcher_(dispatcher), logger_(logger) {
}

bool Engine::loadTheme(const boost::json::object& entry) {
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
    // the first theme loaded is active unless the document names one, which is what
    // makes a config carrying a single theme need no field at all
    if (!activeTheme_) {
        activeTheme_ = theme;
    }
    return true;
}

bool Engine::loadThemes(const boost::json::object& doc) {
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
        const std::string name = boost::json::value_to<std::string>(doc.at("theme"));
        if (!activeTheme(name)) {
            return false;
        }
    }
    return true;
}

boost::shared_ptr<Component> Engine::loadComponent(const boost::json::object& entry) {
    std::string componentType = boost::json::value_to<std::string>(entry.at("type"));
    std::string componentName = boost::json::value_to<std::string>(entry.at("name"));

    boost::shared_ptr<Component> component;
    // read individual component types
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
    } else if (componentType == "vbox" || componentType == "hbox") {
        boost::shared_ptr<component::Box> box = componentType == "vbox"
            ? boost::static_pointer_cast<component::Box>(boost::make_shared<component::VerticalBox>())
            : boost::static_pointer_cast<component::Box>(boost::make_shared<component::HorizontalBox>());
        loadBox(entry, box);
        component = box;
    } else {
        logger_->get()->error("Unrecognized ui component type [{}]", componentType);
        return nullptr;
    }
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
    return component;
}

bool Engine::loadChildren(const boost::json::object& entry, const boost::shared_ptr<Component>& component) {
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

bool Engine::loadContainer(const boost::json::object& entry) {
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

bool Engine::load(const boost::shared_ptr<v3d::asset::Json>& config) {
    auto const doc = config->document();

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

/**
 **/
bool Engine::loadStyle(const boost::json::object& entry, const boost::shared_ptr<style::Theme>& theme) {
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

/**
 **/
boost::shared_ptr<style::Property> Engine::loadProperty(const std::string& section,
    const boost::json::object& property, const std::string& name, std::string* propertyClass) {
    if (section == "colors") {
        *propertyClass = "color";
        return boost::make_shared<style::prop::Color>(name,
            numbers<glm::vec4, 4>(property, "value", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)));
    }
    if (section == "numbers") {
        if (!property.contains("value") || !property.at("value").is_number()) {
            logger_->get()->error("The style number [{}] has no value", name);
            return nullptr;
        }
        *propertyClass = "number";
        return boost::make_shared<style::prop::Number>(name,
            static_cast<float>(boost::json::value_to<double>(property.at("value"))));
    }
    if (section == "fonts") {
        boost::shared_ptr<style::prop::Font> font = boost::make_shared<style::prop::Font>(name,
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
    return boost::make_shared<style::prop::Image>(name,
        boost::json::value_to<std::string>(property.at("source")));
}

bool Engine::loadProperties(const boost::json::object& entry, const boost::shared_ptr<Style>& target) {
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
void Engine::loadAttributes(const boost::json::object& entry, const boost::shared_ptr<Component>& component) {
    loadLayout(entry, &component->layout());
    if (entry.contains("style")) {
        component->style(boost::json::value_to<std::string>(entry.at("style")));
    }
    component->visible(flag(entry, "visible", component->visible()));
    component->pickable(flag(entry, "pickable", component->pickable()));
    if (entry.contains("depth")) {
        component->depth(boost::json::value_to<unsigned int>(entry.at("depth")));
    }
}

/**
 **/
void Engine::loadLayout(const boost::json::object& entry, Layout* layout) {
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
boost::shared_ptr<component::Button> Engine::loadButton(const boost::json::object& entry) {
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
boost::shared_ptr<component::Panel> Engine::loadPanel(const boost::json::object& entry) {
    static_cast<void>(entry);
    // everything a panel is drawn with is its style's, per ADR-0020, so there is nothing
    // of its own to read
    return boost::make_shared<component::Panel>();
}

/**
 **/
boost::shared_ptr<component::Bar> Engine::loadBar(const boost::json::object& entry) {
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
void Engine::loadBox(const boost::json::object& entry, const boost::shared_ptr<component::Box>& box) {
    if (entry.contains("spacing")) {
        box->spacing(static_cast<float>(boost::json::value_to<double>(entry.at("spacing"))));
    }
    box->stretch(flag(entry, "stretch", box->stretch()));
}

/**
 **/
boost::shared_ptr<component::Label> Engine::loadLabel(const boost::json::object& entry) {
    boost::shared_ptr<component::Label> label = boost::make_shared<component::Label>();
    if (entry.contains("label")) {
        label->text(boost::json::value_to<std::string>(entry.at("label")));
    }
    return label;
}

/**
 **/
boost::shared_ptr<component::Icon> Engine::loadIcon(const boost::json::object& entry) {
    if (!entry.contains("source")) {
        logger_->get()->error("An icon names no source");
        return nullptr;
    }
    return boost::make_shared<component::Icon>(boost::json::value_to<std::string>(entry.at("source")));
}

/**
 **/
std::size_t Engine::resolveThemeImages(const Resolve& resolve) {
    std::size_t resolved = 0;
    for (const boost::shared_ptr<style::Theme>& theme : themes_) {
        for (const boost::shared_ptr<Style>& target : theme->getStyleSet("", "")) {
            for (const boost::shared_ptr<style::Property>& property : target->getPropertySet("", "image")) {
                boost::shared_ptr<style::prop::Image> image =
                    boost::dynamic_pointer_cast<style::prop::Image>(property);
                if (!image) {
                    continue;
                }
                const v3d::render::realtime::TextureHandle texture = resolve(std::string(image->source()));
                if (!texture.valid()) {
                    logger_->get()->error("Unable to resolve the ui image [{}]", image->source());
                    continue;
                }
                image->texture(texture);
                resolved++;
            }
        }
    }
    return resolved;
}

std::size_t Engine::resolveComponentImages(const Resolve& resolve, const boost::shared_ptr<Component>& component) {
    std::size_t resolved = 0;

    boost::shared_ptr<component::Icon> icon = boost::dynamic_pointer_cast<component::Icon>(component);
    if (icon) {
        if (resolveIcon(resolve, std::string(icon->source()), icon)) {
            resolved++;
        }
        return resolved;
    }
    boost::shared_ptr<component::Button> button = boost::dynamic_pointer_cast<component::Button>(component);
    if (button) {
        if (resolveIcon(resolve, std::string(button->icon()), button)) {
            resolved++;
        }
        return resolved;
    }
    // a strip's buttons are its own rather than the container's, so they are not
    // reached by walking what the container holds
    boost::shared_ptr<component::Toolbar> bar = boost::dynamic_pointer_cast<component::Toolbar>(component);
    if (!bar) {
        return resolved;
    }
    for (std::size_t index = 0; index < bar->size(); index++) {
        const boost::shared_ptr<component::Button> held = bar->button(index);
        if (held && resolveIcon(resolve, std::string(held->icon()), held)) {
            resolved++;
        }
    }
    return resolved;
}

std::size_t Engine::resolveContainerImages(const Resolve& resolve) {
    std::size_t resolved = 0;
    for (const boost::shared_ptr<Container>& container : containers_) {
        for (const boost::shared_ptr<Component>& component : container->components()) {
            resolved += resolveComponentImages(resolve, component);
        }
    }
    return resolved;
}

std::size_t Engine::resolveImages(const Resolve& resolve) {
    if (!resolve) {
        return 0;
    }
    return resolveThemeImages(resolve) + resolveContainerImages(resolve);
}

/**
 **/
template <typename T>
bool Engine::resolveIcon(const Resolve& resolve, const std::string& source, const boost::shared_ptr<T>& target) {
    // a component naming no image is not a failure - most of them name none
    if (source.empty()) {
        return false;
    }
    const v3d::render::realtime::TextureHandle texture = resolve(source);
    if (!texture.valid()) {
        logger_->get()->error("Unable to resolve the ui image [{}]", source);
        return false;
    }
    target->texture(texture);
    return true;
}

/**
 **/
boost::shared_ptr<component::Menu> Engine::loadMenu(const boost::json::object& entry) {
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

        boost::shared_ptr<component::MenuItem> menuItem = boost::make_shared<component::MenuItem>(menu::stringToType(itemType), label);
        // the owning menu has to be set before the submenu below, which reads it to find its parent
        menuItem->menu(menu);
        if (command.context()) {
            menuItem->event(command);
        }

        menu->addItem(menuItem);

        if (menuItem->type() == menu::ItemType::Submenu) {
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
boost::shared_ptr<component::MenuBar> Engine::loadMenuBar(const boost::json::object& entry) {
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
v3d::event::Event Engine::loadCommand(const boost::json::object& entry) {
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
boost::shared_ptr<component::Toolbar> Engine::loadToolbar(const boost::json::object& entry) {
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

/**
 **/
boost::shared_ptr<style::Theme> Engine::theme(const std::string_view& name) const {
    auto it = themes_.begin();
    for (; it != themes_.end(); ++it) {
        if ((*it)->name() == name) {
            return *it;
        }
    }
    return nullptr;
}

/**
 **/
boost::shared_ptr<style::Theme> Engine::activeTheme() const {
    return activeTheme_;
}

/**
 **/
bool Engine::activeTheme(const std::string_view& name) {
    boost::shared_ptr<style::Theme> found = theme(name);
    if (!found) {
        logger_->get()->error("No such ui theme [{}]", name);
        return false;
    }
    activeTheme_ = found;
    return true;
}

boost::shared_ptr<Container> Engine::container(const std::string_view& name) {
    auto containers = containers_.begin();
    for (; containers != containers_.end(); ++containers) {
        if ((*containers)->name() == name) {
            return *containers;
        }
    }
    return nullptr;
}
/**
 **/
const std::vector<boost::shared_ptr<Container>>& Engine::containers() const noexcept {
    return containers_;
}

};  // namespace v3d::ui
