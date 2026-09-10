/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/geometry/Bound2D.h>
#include <api/ui/Component.h>

#include <cstddef>
#include <string>
#include <vector>

#include "Menu.h"
#include "MenuItem.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::ui::component {

/**
 * A row of named menus along the top of the window, driven by the cursor.
 *
 * One of the menus may be open, and under it any depth of submenu the cursor has
 * descended into - panels() is that stack, outermost first, and is what a renderer draws
 * and what the cursor is tested against.
 *
 * The bar answers where the cursor is out of the bounds a renderer left on the
 * components, per ADR-0019, so nothing is hit until something has been drawn.
 **/
class MenuBar : public Component {
 public:
    MenuBar();
    ~MenuBar() = default;

    /**
     * Add a menu to the end of the row.
     * @param label what the bar shows for it
     * @param menu the menu that drops down from it
     **/
    void add(const std::string& label, const boost::shared_ptr<Menu>& menu);

    /**
     * @return how many menus are in the row
     **/
    std::size_t size() const noexcept;

    /**
     * @param index which menu, which must be less than size()
     * @return the label the bar shows for it
     **/
    const std::string& label(std::size_t index) const;

    /**
     * @param index which menu, which must be less than size()
     * @return the menu that drops down from it
     **/
    boost::shared_ptr<Menu> menu(std::size_t index) const;

    /**
     * Where a renderer put one menu's label in the strip.
     *
     * The strip is the bar's own drawing, so the strip's layout is the bar's state: a
     * menu is drawn twice - once as a label up here and again as the panel it drops - and
     * its own bounds are the panel.
     *
     * @param index which menu, which must be less than size()
     **/
    void place(std::size_t index, const glm::vec2& position, const glm::vec2& size);

    /**
     * @param index which menu, which must be less than size()
     * @return the label's bounds as the last draw left them
     **/
    v3d::type::geometry::Bound2D bound(std::size_t index) const;

    /**
     * @return the menu the cursor is over, or -1 when it is over none
     **/
    int hover() const noexcept;

    /**
     * @return the open menu, or -1 when the bar is closed
     **/
    int open() const noexcept;

    /**
     * Open one of the menus, closing whatever was open.
     * @param index which menu, or -1 to close the bar
     **/
    void open(int index);

    /**
     * Close the bar, leaving the cursor's hover where it is.
     **/
    void close();

    /**
     * @return whether the bar has a menu open, which is when it takes the cursor
     **/
    bool active() const noexcept;

    /**
     * The open menu and every submenu under it, outermost first. Empty when closed.
     **/
    const std::vector<boost::shared_ptr<Menu>>& panels() const noexcept;

    /**
     * The cursor moved.
     * @param cursor where it is, in the pixels the bar was drawn in
     * @return whether it is over the bar or one of its open panels
     **/
    bool motion(const glm::vec2& cursor);

    /**
     * The primary button went down.
     *
     * A press on a menu opens or closes it, one on a submenu item descends, and one on
     * any other item sends that item's command and closes the bar. A press anywhere else
     * while the bar is open closes it and is consumed, so the click that dismisses a menu
     * does nothing else.
     *
     * @param cursor where the cursor is
     * @return whether the bar took the press
     **/
    bool press(const glm::vec2& cursor);

    /**
     * Find an item anywhere in the bar by the command it sends, so that whatever answers
     * a command can mark the item that names it.
     * @param command the item's event as Event::str() gives it - "context::name"
     * @return the item, or null when no item sends that command
     **/
    boost::shared_ptr<MenuItem> find(const std::string& command) const;

 private:
    /**
     * @return the index of the menu whose label the cursor is on, or -1
     **/
    int labelAt(const glm::vec2& cursor) const;

    /**
     * @return the index of the item the cursor is on within a panel, or -1
     **/
    static int itemAt(const boost::shared_ptr<Menu>& panel, const glm::vec2& cursor);

    /**
     * @return whether a component's drawn bounds contain the cursor
     **/
    static bool within(const Component& component, const glm::vec2& cursor);

    /**
     * Leave the panel stack this many deep, closing anything under it.
     **/
    void truncate(std::size_t depth);

    /**
     * Descend into an item's submenu when it has one, dropping any deeper panel.
     * @param depth the panel the item belongs to
     **/
    void descend(std::size_t depth, const boost::shared_ptr<MenuItem>& item);

    /**
     * Search one menu and everything below it.
     **/
    static boost::shared_ptr<MenuItem> find(const boost::shared_ptr<Menu>& menu, const std::string& command);

    std::vector<std::string> labels_;
    std::vector<v3d::type::geometry::Bound2D> bounds_;  // where each label was drawn in the strip
    std::vector<boost::shared_ptr<Menu>> menus_;
    std::vector<boost::shared_ptr<Menu>> panels_;
    int hover_;  // the menu the cursor is over, or -1
    int open_;   // the open menu, or -1
};

};  // namespace v3d::ui::component
