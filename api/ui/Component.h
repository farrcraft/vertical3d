/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/geometry/Bound2D.h>
#include <api/ui/component/Type.h>

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/glm.hpp>

#include "Layout.h"

namespace v3d::ui {

/**
 * The base class of every ui component.
 *
 * A component holds other components, and layout() says where it sits in the one holding
 * it. position() and size() are the box it was last drawn in. The layout pass writes them
 * when it resolves layout(), and the cursor is tested against them.
 */
class Component {
 public:
    explicit Component(component::Type type);
    virtual ~Component();
    // a copy would share the children it was copied from, and they would still name the
    // original as their parent
    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;

    /**
     * Get the id of the component, which is unique among the components built so far and
     * means nothing beyond that.
     * @return the id
     */
    unsigned int id() const;

    /**
     * Set the position of the component.
     * @param pos the new position
     */
    void position(const glm::vec2& pos);
    /**
     * Set the size of the component.
     * @param s the new size
     */
    void size(const glm::vec2& s);
    /**
     * Get the current position of the component.
     * @return the current position
     */
    glm::vec2 position() const;
    /**
     * Get the current size of the component.
     * @return the current size
     */
    glm::vec2 size() const;
    /**
     * Get the component's bounding volume
     * @return the component's bounding box
     */
    v3d::type::geometry::Bound2D bound() const;
    /**
     * Get the component's z index depth value
     * @return the component's zindex
     */
    unsigned int depth() const;
    /**
     * Set the style name of the component
     * @param str the style name
     */
    void style(const std::string& str);
    /**
     * Get the style name of the component
     * @return the style name
     */
    std::string_view style() const;
    /**
     * Get whether the component is visible or not
     * @return true if the component is visible
     */
    bool visible() const;
    /**
     * Set the visibility state of the component
     * @param vis the new visibility setting
     */
    void visible(bool vis);
    /**
     * Get the component name
     * @return the component name
     */
    std::string_view name() const;
    /**
     * Set the component name
     * @param str the new component name
     */
    void name(const std::string& str);

    /**
     * Get the component type
     * @return the component type
     **/
    component::Type type() const;

    /**
     * Set the component's z index depth value. A container draws in order of depth, and
     * equal depths keep the order they were added in.
     * @param index the new depth
     **/
    void depth(unsigned int index);

    /**
     * @return where this component is placed and how big it is, to be changed in place
     **/
    Layout& layout() noexcept;
    const Layout& layout() const noexcept;

    /**
     * Hold another component inside this one. The child is laid out against this
     * component's box and drawn after it.
     *
     * A component is held by exactly one parent; adding one that already has another
     * leaves it in the first.
     **/
    void add(const boost::shared_ptr<Component>& child);

    /**
     * @return what this component holds, in the order it was added
     **/
    const std::vector<boost::shared_ptr<Component>>& children() const noexcept;

    /**
     * @return the component this one is laid out inside, or null when it is a root
     **/
    Component* parent() const noexcept;

    /**
     * Make this component the parent of an item it holds outside children(), such as a
     * toolbar's buttons or a menu's items. usable() then walks up through the holder, so a
     * disabled toolbar disables its buttons. The holder lays out and draws the item itself,
     * so the item is not a child, and no loop over children() reaches it.
     *
     * An item already held by something else is left with it, as add() leaves a child.
     **/
    void adopt(Component& item) noexcept;

    /**
     * Release an item adopt() took, so it no longer names this as its parent. A holder calls
     * this for each item from its own destructor. An app may still hold an item after its
     * holder is gone, and usable() on that item must not reach the destroyed holder.
     **/
    void disown(Component& item) noexcept;

    /**
     * Get whether the component takes the cursor.
     *
     * False by default, because a hud is mostly labels and bars drawn over a scene that has
     * to stay clickable. A component takes a press only when this is set. Controls set it in
     * their constructors.
     **/
    bool pickable() const;
    void pickable(bool pick);

    /**
     * Get whether the component takes the keyboard.
     *
     * False by default, because most of a ui reads no keys. Controls that read keys set it in
     * their constructors. A press on a component that is not focusable takes the focus off
     * whatever had it.
     **/
    bool focusable() const;
    void focusable(bool takes);

    /**
     * Get whether the keyboard is on this component.
     *
     * Written by Interface::focus(), which keeps one component focused at a time. The flag is
     * stored on the component so that drawing a component reads only the component.
     **/
    bool focused() const;
    void focused(bool on);

    /**
     * Get whether what this component holds is cut off at its box.
     *
     * False by default, because a menu drops a panel outside the strip it came from, and a
     * badge can sit half outside the panel it belongs to. A component that holds more than it
     * can show, such as a list or a scrolled panel, sets it.
     **/
    bool clip() const;
    void clip(bool cut);

    /**
     * Get whether the component can be used.
     *
     * True by default. The cursor never changes it. A disabled component is not picked, is
     * skipped by the tab order, and is drawn in the theme's disabled colour. Disabling a
     * component disables everything it holds, so a screen greys out a group of controls by
     * disabling the box around them.
     **/
    bool enabled() const;
    void enabled(bool on);

 private:
    Layout layout_;
    std::vector<boost::shared_ptr<Component>> children_;
    Component* parent_;
    glm::vec2 position_;
    glm::vec2 size_;
    unsigned int zIndex_;
    unsigned int id_;
    std::string style_;
    std::string name_;
    bool visible_;
    bool enabled_;
    bool pickable_;
    bool focusable_;
    bool focused_;
    bool clip_;
    component::Type type_;
};

/**
 * Sort components into the order they are drawn: by z index, keeping the order they were
 * added in between equal depths. A null entry is left out of the copy.
 *
 * @return a sorted copy, lowest depth first, so the last is drawn on top
 **/
std::vector<boost::shared_ptr<Component>> ordered(const std::vector<boost::shared_ptr<Component>>& components);

/**
 * Whether components are already in the order they are drawn, so that a loop over them
 * needs no sorted copy.
 *
 * True whenever nothing has been given a depth, which is the common case. The check is one
 * scan, with no allocation or sort, and runs once per parent per frame.
 **/
bool inDrawOrder(const std::vector<boost::shared_ptr<Component>>& components) noexcept;

/**
 * Whether a component can be used, which is whether it and everything holding it are
 * enabled.
 *
 * Picking and the tab order skip a disabled subtree whole, so they do not call this.
 * Drawing visits every component, so it calls this, and a label inside a disabled box is
 * drawn greyed.
 **/
bool usable(const Component& component) noexcept;

};  // end namespace v3d::ui
