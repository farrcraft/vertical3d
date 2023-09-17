/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::ui::component {
    /**
     **/
    enum class Type {
        TYPE_UNDEFINED = 0,
        MENU = 1,
        MENUBAR = 2,
        MENU_ITEM = 3,
        POPUP_MENU = 4,
        RADIAL_MENU = 5,
        BUTTON = 6,
        CHECKBOX = 7,
        DIALOG = 8,
        FRAME = 9,
        HORIZONTAL_FRAME = 10,
        VERTICAL_FRAME = 11,
        ICON = 12,
        INPUT_BOX = 13,
        LABEL = 14,
        RADIO_BUTTON = 15,
        SCROLLBAR = 16,
        SELECT_LIST = 17,
        SPINNER = 18,
        TAB_BAR = 19,
        TAB_PAGE = 20,
        TEXT_BOX = 21,
        TOOLTIP = 22
    };
}  // namespace v3d::ui::component
