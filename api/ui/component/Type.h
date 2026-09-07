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
    TYPE_UNDEFINED,
    MENU,
    MENUBAR,
    MENU_ITEM,
    BUTTON,
    CHECKBOX,
    HORIZONTAL_FRAME,
    VERTICAL_FRAME,
    ICON,
    LABEL,
    RADIO_BUTTON,
    SCROLLBAR,
    SELECT_LIST,
    TAB_BAR,
    TAB_PAGE,
    TOOLBAR,
    PANEL,
    BAR
};

}  // namespace v3d::ui::component
