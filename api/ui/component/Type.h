/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string_view>

namespace v3d::ui::component {

/**
 * What a component is: what drawing switches on and what the loader builds.
 *
 * Every one of these has a loader and a draw path. A type is not listed here until it has
 * both.
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
    Slider,
    TabBar,
    TabPage,
    TextBox,
    Toolbar,
    VerticalBox
};

/**
 * What a ui config's "type" calls this component.
 *
 * This is the only place the config's names are written down. It is an exhaustive switch
 * with no default, so a type added to the enum above fails the build here (C4062) until it
 * is given a name. A type a config cannot ask for returns empty: a menu item is built by the
 * menu that holds it, and Undefined is not a component.
 **/
std::string_view name(Type type);

/**
 * The reverse: what a config asked for.
 *
 * @return the type, or Undefined for a name no component has. The loader reports that as
 *         an unrecognised type.
 **/
Type parse(std::string_view text);

/**
 * What a type is, for a rule that holds of a kind of component rather than of one.
 **/
struct Traits final {
    bool strip = false;   /**< a menu bar or a toolbar: stacked at an edge, its items its own **/
    bool flow = false;    /**< a component::Box, laying its children out in the order it holds them **/
    bool pages = false;   /**< holds pages and shows one, so only that one is live **/
    bool text = false;    /**< takes typed text while it has the focus **/
};

/**
 * The traits of a type. An exhaustive switch like name(), so a type added to the enum fails
 * the build here until its traits are given. A rule about a kind of component reads these
 * traits rather than testing for particular types.
 **/
Traits traits(Type type);

}  // namespace v3d::ui::component
