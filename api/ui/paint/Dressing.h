/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec4.hpp>

namespace v3d::ui::paint {

/**
 * The colours and metrics a component is drawn with.
 *
 * A Style is a set of properties that a theme holds and that are looked up by name. A
 * Dressing is what a Style resolves to: a plain struct with no strings in it. Drawing a
 * component reads its fields and does no lookups.
 *
 * A theme's "ui" style sets the defaults, and a component's own style class sets what
 * differs from them. A value that no style sets keeps its default, so an empty theme
 * changes nothing. style::Resolver builds a Dressing.
 **/
struct Dressing final {
    Dressing() noexcept;

    float lineHeight;      /**< the height of one line of text: a menu item, a list row, a label row **/
    float padding;         /**< the gap between the text and the panel around it **/
    float barHeight;       /**< how tall the strip of a menu bar or a toolbar is **/
    float iconSize;        /**< the side of the square a button's icon is drawn in **/
    float panelPadding;    /**< the gap above and below the items of a dropped panel **/
    float scrollbarWidth;  /**< how thick a scrollbar is across its direction **/
    float markSize;        /**< the side of the box, or the width of the disc, a mark sits in **/
    float borderWidth;     /**< how thick a panel's or a bar's outline is drawn **/
    float focusWidth;      /**< how thick the ring around the focused component is **/
    float radius;          /**< how far a panel's corners are rounded, 0 for square **/
    glm::vec4 panel;         /**< the background of a panel, a menu, a list or a text box **/
    glm::vec4 border;        /**< an outline **/
    glm::vec4 track;         /**< the unfilled part of a bar **/
    glm::vec4 fill;          /**< the filled part of a bar **/
    glm::vec4 thumb;         /**< the part of a scrollbar's track that is taken hold of **/
    glm::vec4 mark;          /**< what a checked box or a chosen radio button is marked with **/
    glm::vec4 caret;         /**< a text box's caret **/
    glm::vec4 placeholder;   /**< what an empty text box shows in place of text **/
    glm::vec4 tab;           /**< a tab that is not the chosen one **/
    glm::vec4 text;          /**< ordinary text **/
    glm::vec4 activeText;    /**< the text of the chosen item: the active menu item, the chosen row or tab **/
    glm::vec4 disabledText;  /**< the text of a control that cannot be used **/
    glm::vec4 highlight;     /**< what is drawn behind the chosen item, and behind selected text **/
    glm::vec4 hover;         /**< what is drawn behind a toolbar button the cursor is on **/
    glm::vec4 focus;         /**< the ring around whatever the keyboard is on **/
};

};  // namespace v3d::ui::paint
