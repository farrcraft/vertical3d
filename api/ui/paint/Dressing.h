/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec4.hpp>

namespace v3d::ui::paint {

/**
 * The colours and metrics the retained components and the immediate layer are drawn with.
 *
 * A Style is a set of properties that a theme holds and that are looked up by name. A
 * Dressing is what a Style resolves to: a plain struct with no strings in it. Drawing reads
 * its fields and does no lookups.
 *
 * Both ui models use this one struct, and a theme names each field by the same key for
 * both. Each model reads only the fields it draws with. The two start from different sizes,
 * because a hud is read at a glance and a tool panel is read closely: the default
 * constructor gives the retained ui's values, and tools() gives the immediate layer's.
 *
 * A theme's "ui" style sets the retained defaults, and its "tools" style sets the immediate
 * ones. A component's own style class sets what differs from the "ui" values. A value that
 * no style sets keeps its default, so an empty theme changes nothing.
 **/
struct Dressing final {
    Dressing() noexcept;

    /**
     * @return the defaults the immediate layer starts from
     **/
    static Dressing tools() noexcept;

    float lineHeight;      /**< the height of one line of text: a menu item, a list row, a label row **/
    float padding;         /**< the gap between the text and the panel or widget around it **/
    float spacing;         /**< the gap between one immediate widget and the next **/
    float barHeight;       /**< how tall a menu bar, a toolbar, a title bar, a tab or an immediate button is **/
    float iconSize;        /**< the side of the square a button's icon is drawn in **/
    float panelPadding;    /**< the gap above and below the items of a dropped panel **/
    float scrollbarWidth;  /**< how thick a scrollbar is across its direction **/
    float markSize;        /**< the side of the box, or the width of the disc, a mark sits in **/
    float borderWidth;     /**< how thick a panel's, a window's or a bar's outline is drawn **/
    float focusWidth;      /**< how thick the ring around the focused component is **/
    float radius;          /**< how far a panel's or a window's corners are rounded, 0 for square **/
    float indent;          /**< how far a bullet pushes its text in **/
    glm::vec4 panel;         /**< the background of a panel, a window, a menu, a list or a text box **/
    glm::vec4 border;        /**< an outline **/
    glm::vec4 titleBar;      /**< the band across the top of a window, and a table's header **/
    glm::vec4 track;         /**< the unfilled part of a bar or a scrollbar, and the box a mark sits in **/
    glm::vec4 fill;          /**< the filled part of a bar **/
    glm::vec4 thumb;         /**< the part of a scrollbar's track that is taken hold of **/
    glm::vec4 mark;          /**< what a checked box or a chosen radio button is marked with **/
    glm::vec4 caret;         /**< a text field's caret **/
    glm::vec4 placeholder;   /**< what an empty text field shows in place of text **/
    glm::vec4 tab;           /**< a tab that is not the chosen one **/
    glm::vec4 widget;        /**< the face of an immediate button, field or check box at rest **/
    glm::vec4 rule;          /**< a separator, and the line under an immediate tab strip **/
    glm::vec4 text;          /**< ordinary text **/
    glm::vec4 activeText;    /**< the text of the chosen or hovered item: a menu item, a row, a tab, a button **/
    glm::vec4 disabledText;  /**< the text of a control that cannot be used, and secondary text **/
    glm::vec4 highlight;     /**< what is drawn behind the chosen item, and behind selected text **/
    glm::vec4 hover;         /**< what is drawn behind the item the cursor is on **/
    glm::vec4 focus;         /**< the ring around whatever the keyboard is on **/
};

};  // namespace v3d::ui::paint
