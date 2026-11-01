/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/geometry/Bound2D.h>
#include <api/ui/Component.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Menu.h"
#include "Item.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::ui::component::menu {

/**
 * A row of named menus, driven by the cursor.
 *
 * A docked bar, which is the default, runs along the top of the window and takes that strip
 * from what is laid out below it. A bar that is not docked is laid out by its Layout like
 * any other component, as a header over part of the window, and takes nothing from the rest.
 * Either drops its panels from its own bottom edge.
 *
 * One of the menus may be open, and under it any depth of submenu the cursor has
 * descended into. panels() is that stack, outermost first. A renderer draws it, and the
 * cursor is tested against it.
 *
 * The bar tests the cursor against the bounds a renderer left on the components, so
 * nothing is hit until something has been drawn.
 *
 * It is pickable from the start. The cursor skips a menu bar that is set not pickable.
 **/
class Bar : public Component {
 public:
    Bar();
    ~Bar() override;

    /**
     * Add a menu to the end of the row. A null menu is ignored, and the row is left as it was.
     * @param label what the bar shows for it
     * @param menu the menu that drops down from it
     **/
    void add(const std::string& label, const boost::shared_ptr<Menu>& menu);

    /**
     * @return how many menus are in the row
     **/
    std::size_t count() const noexcept;

    /**
     * @param index which menu, which must be less than count()
     * @return the label the bar shows for it
     **/
    const std::string& label(std::size_t index) const;

    /**
     * @param index which menu, which must be less than count()
     * @return the menu that drops down from it
     **/
    boost::shared_ptr<Menu> menu(std::size_t index) const;

    /**
     * Where a renderer put one menu's label in the strip.
     *
     * A menu is drawn twice, as a label in the strip and as the panel it drops, and one
     * component cannot hold two rectangles. The bar holds the labels' bounds, and each
     * menu's own bounds are its panel.
     *
     * @param index which menu, which must be less than count()
     **/
    void place(std::size_t index, const glm::vec2& position, const glm::vec2& size);

    /**
     * @param index which menu, which must be less than count()
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
     * @return whether the bar has a menu open. The bar takes the cursor while it does
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
     * How long the cursor has to rest on a submenu's item before the submenu opens, in
     * seconds. A press on the item opens it at once, and moving onto an item with no submenu
     * closes any deeper panel at once. Zero, the default, opens a submenu as soon as the
     * cursor reaches its item.
     **/
    void delay(float seconds) noexcept;

    /**
     * Whether the bar runs along the top of the window, or is laid out by its Layout.
     **/
    void docked(bool along) noexcept;
    bool docked() const noexcept;

    /**
     * How much of the bar's strip is drawn over what is beneath it, from 0 to 1. Its titles are
     * drawn whole. A value outside the range is clamped to it.
     **/
    void opacity(float drawn) noexcept;
    float opacity() const noexcept;

    /**
     * Open the submenu the cursor has rested on for the delay. Called once a frame with the
     * ui's time. The first call after the cursor reaches an item starts its wait.
     **/
    void settle(float now);

    /**
     * The primary button went down.
     *
     * A press on a menu opens or closes it, one on a submenu item descends, and one on
     * any other item sends that item's command and closes the bar. A press anywhere else
     * while the bar is open closes it and is consumed, so the click that dismisses a menu
     * does nothing else. A press on the bar beside its labels is not taken while it is closed.
     *
     * @param cursor where the cursor is
     * @return whether the bar took the press
     **/
    bool press(const glm::vec2& cursor);

    /**
     * A key went down while the bar is open. The deepest panel open is the one walked.
     *
     * The down and up arrows choose the next and the previous item, stepping over separators
     * and wrapping. The right arrow opens the chosen item's submenu at once, or opens the next
     * menu when the item has none. The left arrow closes the deepest submenu, or opens the
     * previous menu from the first panel. A menu opened from the keys chooses its first item.
     * Return and space act on the chosen item as a press would. Escape closes the deepest
     * submenu, or the bar from the first panel.
     *
     * @param key the name api/input gives it
     * @return whether the bar took it, which a closed bar never does
     **/
    bool key(std::string_view key);

    /**
     * Find an item anywhere in the bar by the command it sends, so that whatever handles
     * a command can mark the item that sends it.
     * @param command the item's event as Event::str() returns it, such as "context::name"
     * @return the item, or null when no item sends that command
     **/
    boost::shared_ptr<Item> find(const std::string& command) const;

 private:
    /**
     * A submenu's item the cursor has reached, waiting out the delay before it opens.
     **/
    struct Pending final {
        std::size_t depth = 0;            /**< the panel the item belongs to **/
        boost::weak_ptr<Item> item;
        float since = 0.0F;               /**< when the wait began, once settle() has seen it **/
        bool started = false;
    };

    /**
     * The cursor reached an item of a panel: open its submenu, or wait to, or close what is
     * under the panel when it has none.
     **/
    void reach(std::size_t depth, const boost::shared_ptr<Item>& item);

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
     * Choose the item a number of steps from the one a panel has chosen, stepping over
     * separators and wrapping. A panel with nothing chosen starts before its first item.
     **/
    static void step(const boost::shared_ptr<Menu>& panel, int by);

    /**
     * Open a menu and choose its first item.
     **/
    void openFirst(int index);

    /**
     * Leave the panel stack this many deep, closing anything under it.
     **/
    void truncate(std::size_t depth);

    /**
     * Descend into an item's submenu when it has one, dropping any deeper panel.
     * @param depth the panel the item belongs to
     **/
    void descend(std::size_t depth, const boost::shared_ptr<Item>& item);

    /**
     * Search one menu and everything below it.
     **/
    static boost::shared_ptr<Item> find(const boost::shared_ptr<Menu>& menu, const std::string& command);

    std::vector<std::string> labels_;
    std::vector<v3d::type::geometry::Bound2D> bounds_;  // where each label was drawn in the strip
    std::vector<boost::shared_ptr<Menu>> menus_;
    std::vector<boost::shared_ptr<Menu>> panels_;
    int hover_;  // the menu the cursor is over, or -1
    int open_;   // the open menu, or -1
    float delay_;
    bool docked_ = true;
    float opacity_ = 1.0F;
    std::optional<Pending> pending_;
};

};  // namespace v3d::ui::component::menu
