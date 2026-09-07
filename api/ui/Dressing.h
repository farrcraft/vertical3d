/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec4.hpp>

namespace v3d::ui {

/**
 * What the ui cannot work out from the components alone: the colours and metrics a
 * component is drawn with.
 *
 * Not a Style, which is the bag of properties a theme holds and which is looked up by
 * name. This is what one resolves to - a plain struct with no strings in it, so that
 * drawing a component reads fields rather than asking a map for them.
 *
 * A theme's "ui" style names the defaults, and a component's own style class names what
 * differs from them, per ADR-0020. What a style does not name keeps the value it had, so
 * a theme carrying nothing changes nothing. style::Resolver is what works one out.
 **/
struct Dressing final {
    Dressing() noexcept;

    float lineHeight;      /**< the baseline to baseline distance of one menu item **/
    float padding;         /**< the gap between the text and the panel around it **/
    float barHeight;       /**< how tall the strip of a menu bar or a toolbar is **/
    float iconSize;        /**< the side of the square an icon is drawn in **/
    float panelPadding;    /**< the gap above and below the items of a dropped panel **/
    float scrollbarWidth;  /**< how thick a scrollbar is across its direction **/
    float markSize;        /**< the side of the box, or the width of the disc, a mark sits in **/
    float borderWidth;     /**< how thick a panel's or a bar's outline is drawn **/
    float radius;          /**< how far a panel's corners are rounded, 0 for square **/
    glm::vec4 panel;       /**< the background the menu is drawn on **/
    glm::vec4 border;      /**< the panel's outline **/
    glm::vec4 track;       /**< the unfilled part of a bar **/
    glm::vec4 fill;        /**< the filled part of a bar **/
    glm::vec4 thumb;       /**< the part of a scrollbar's track that is taken hold of **/
    glm::vec4 mark;        /**< what a checked box or a chosen radio button is marked with **/
    glm::vec4 text;        /**< an ordinary item's label **/
    glm::vec4 activeText;  /**< the label of the item navigation is on **/
    glm::vec4 highlight;   /**< what is drawn behind that item **/
    glm::vec4 hover;       /**< what is drawn behind a toolbar button the cursor is on **/
};

};  // namespace v3d::ui
