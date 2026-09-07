/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/glm.hpp>

#include "Layout.h"
#include "component/Type.h"
#include "style/Theme.h"

#include "../../api/type/Bound2D.h"

namespace v3d::ui {

/**
 * A vGUI Component
 * All UI components are all derived from this class.
 *
 * A component holds other components, and layout() says where it sits in the one holding
 * it. position() and size() are the box it was last drawn in - the output of the walk
 * that resolves layout(), and what the cursor is tested against, per ADR-0019 and
 * ADR-0034.
 */
class Component {
 public:
    explicit Component(component::Type type);
    virtual ~Component();

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
    v3d::type::Bound2D bound() const;
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
     * Set the component's z index depth value, which is what a container draws in order
     * of. Equal depths keep the order they were added in.
     * @param index the new depth
     **/
    void depth(unsigned int index);

    /**
     * @return where this component asks to be, to be changed in place
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
     * Get whether the component answers the cursor.
     *
     * False by default, and deliberately: a hud is mostly labels and bars drawn over a
     * scene that has to stay clickable, so a component takes a press only when it was
     * asked to. ADR-0034.
     **/
    bool pickable() const;
    void pickable(bool pick);

    /**
     * Get whether the component takes the keyboard.
     *
     * False by default, for the reason pickable() is: most of a ui has nothing to type
     * into, and a press that took the keyboard off a text box every time it landed on a
     * panel would make one unusable. A component that reads keys - a text box - asks for
     * it. ADR-0040.
     **/
    bool focusable() const;
    void focusable(bool takes);

    /**
     * Get whether the keyboard is on this component.
     *
     * Written by Engine::focus(), which is what keeps one component focused at a time.
     * It is a flag here rather than a question for the engine so that drawing a component
     * reads the component, per ADR-0019.
     **/
    bool focused() const;
    void focused(bool on);

    /**
     * Get whether what this component holds is cut off at its box.
     *
     * False by default, and deliberately: a menu drops a panel out of the strip it came
     * from, and a badge sits half outside the plate it belongs to. A component that holds
     * more than it can show - a list, a scrolled panel - asks for it. ADR-0037.
     **/
    bool clip() const;
    void clip(bool cut);

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
    bool pickable_;
    bool focusable_;
    bool focused_;
    bool clip_;
    component::Type type_;
};

/**
 * Sort components into the order they are drawn: by z index, keeping the order they were
 * added in between equal depths.
 *
 * @return a sorted copy, deepest first
 **/
std::vector<boost::shared_ptr<Component>> ordered(const std::vector<boost::shared_ptr<Component>>& components);

/**
 * Whether components are already in the order they are drawn, so that a walk over them
 * needs no sorted copy of its own.
 *
 * True whenever nothing has been given a depth, which is every container in the tree
 * today - so the common case costs a scan rather than an allocation and a sort, once per
 * parent per frame.
 **/
bool inDrawOrder(const std::vector<boost::shared_ptr<Component>>& components) noexcept;

};  // end namespace v3d::ui
