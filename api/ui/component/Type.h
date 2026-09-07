/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

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

}  // namespace v3d::ui::component
