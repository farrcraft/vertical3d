/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string_view>

namespace v3d::ui::component {

/**
 * What a component is, which is what the draw walk switches on and what the loader builds.
 *
 * Every one of these has a loader and a draw path. A type with neither is a claim the
 * library does not answer for, so it is not listed here until it does.
 **/
enum class Type {
    Undefined,
    Bar,
    Button,
    CheckBox,
    HorizontalBox,
    Icon,
    Label,
    Menu,
    MenuBar,
    MenuItem,
    Panel,
    RadioButton,
    Scrollbar,
    SelectList,
    TabBar,
    TabPage,
    TextBox,
    Toolbar,
    VerticalBox
};

/**
 * What a ui config's "type" calls this component, per ADR-0047.
 *
 * The one place the config's vocabulary is written down, and an exhaustive switch, so a type
 * added to the enum above names this function until it is given a name here. A type a config
 * cannot ask for answers empty - a menu item is built by the menu that holds it, and Undefined
 * is not a component.
 **/
std::string_view name(Type type);

/**
 * The reverse: what a config asked for.
 *
 * @return the type, or Undefined for a name no component answers to - which is what the
 *         loader reports as an unrecognised type rather than building nothing quietly
 **/
Type parse(std::string_view text);

}  // namespace v3d::ui::component
