/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Type.h"

#include <string_view>

namespace v3d::ui::component {

std::string_view name(Type type) {
    switch (type) {
        case Type::Bar:            return "bar";
        case Type::Button:         return "button";
        case Type::CheckBox:       return "checkbox";
        case Type::HorizontalBox:  return "hbox";
        case Type::Icon:           return "icon";
        case Type::Label:          return "label";
        case Type::Menu:           return "menu";
        case Type::MenuBar:        return "menubar";
        case Type::Panel:          return "panel";
        case Type::RadioButton:    return "radio";
        case Type::Scrollbar:      return "scrollbar";
        case Type::SelectList:     return "list";
        case Type::Slider:         return "slider";
        case Type::TabBar:         return "tabs";
        case Type::TabPage:        return "tab";
        case Type::TextBox:        return "textbox";
        case Type::Toolbar:        return "toolbar";
        case Type::VerticalBox:    return "vbox";
        case Type::MenuItem:
            // built by the menu that holds it, from that menu's own entry, so a config never
            // names one as a component of its own
        case Type::Undefined:
            return std::string_view();
    }
    // every enumerator is handled above and the switch carries no default, so C4062 names
    // this function when a component type is added - see ADR-0047
    return std::string_view();
}

Traits traits(Type type) {
    Traits traits;
    switch (type) {
        case Type::MenuBar:
        case Type::Toolbar:
            traits.strip = true;
            break;
        case Type::HorizontalBox:
        case Type::VerticalBox:
            traits.flow = true;
            break;
        case Type::TabBar:
            traits.pages = true;
            break;
        case Type::TextBox:
            traits.text = true;
            break;
        case Type::Undefined:
        case Type::Bar:
        case Type::Button:
        case Type::CheckBox:
        case Type::Icon:
        case Type::Label:
        case Type::Menu:
        case Type::MenuItem:
        case Type::Panel:
        case Type::RadioButton:
        case Type::Scrollbar:
        case Type::SelectList:
        case Type::Slider:
        case Type::TabPage:
            break;
    }
    return traits;
}

Type parse(std::string_view text) {
    // walked rather than mapped: the list is short, this runs once per component in a config
    // read at startup, and a table would be a second place to forget. A type with no config
    // name answers empty from name(), which no non-empty text matches.
    //
    // The walk runs to VerticalBox because the enum is kept alphabetical and that is its last
    // entry. A type added past it would be skipped here while compiling everywhere else, so
    // TypeTest sweeps wider than the enum and fails if one ever is
    if (text.empty()) {
        return Type::Undefined;
    }
    for (int index = static_cast<int>(Type::Undefined) + 1;
        index <= static_cast<int>(Type::VerticalBox); ++index) {
        const Type type = static_cast<Type>(index);
        if (name(type) == text) {
            return type;
        }
    }
    return Type::Undefined;
}

};  // namespace v3d::ui::component
