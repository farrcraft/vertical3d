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
    Slider,
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
 * The traits of a type - an exhaustive switch like name(), so a type added to the enum names
 * this function too and says what it is, rather than being left out of a rule written as a
 * test against the types somebody remembered. ADR-0047.
 **/
Traits traits(Type type);

}  // namespace v3d::ui::component
