/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>
#include <api/type/geometry/Bound2D.h>
#include <api/ui/paint/Text.h>

#include <functional>
#include <string>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::canvas {
class Canvas;
};  // namespace v3d::render::realtime::canvas

namespace v3d::ui {
class Arranger;
};  // namespace v3d::ui

namespace v3d::ui::component::menu {
class Bar;
class Item;
class Menu;
};  // namespace v3d::ui::component::menu

namespace v3d::ui::style {
class Resolver;
};  // namespace v3d::ui::style

namespace v3d::ui::paint {
struct Dressing;
};  // namespace v3d::ui::paint

namespace v3d::ui::paint::internal {

class Easing;

/**
 * Draws menus: a game menu's active level centred on the canvas, and a menu bar with the
 * panels it drops and the flyouts out of those.
 *
 * Every menu is drawn with the base dressing. A menu has no style class of its own.
 **/
class MenuRenderer final {
 public:
    /**
     * How a command's shortcut is shown, such as "Ctrl+Z", or empty when nothing binds it.
     **/
    typedef std::function<std::string(const v3d::event::Event& command)> Shortcut;

    /**
     * @param measure how wide a string is when the app draws it
     * @param write how the app draws a string
     * @param styles where the base dressing comes from
     * @param arranger what places a menu's panels and items
     * @param easing the clock a highlight eases and a panel opens on
     *
     * The last three are held by reference, because the renderer owns them.
     **/
    MenuRenderer(const Measure& measure, const Write& write, const style::Resolver& styles,
        const Arranger& arranger, const Easing& easing);

    /**
     * Look up the shortcut a menu item shows beside its label. Without one, nothing shows a
     * shortcut.
     **/
    void shortcuts(const Shortcut& lookup);

    /**
     * Set a centred menu's title and its chosen item apart from the body text.
     **/
    void emphasis(const Emphasis& sets);

    /**
     * @return the shortcut of the command a component sends, or empty when it sends none or
     *         nothing binds it
     **/
    std::string shortcut(const v3d::event::Event& command) const;

    /**
     * Draw the active level of a menu, centred in the canvas's safe area, under its title.
     *
     * It is the active level rather than the menu itself that is drawn: descending into
     * a submenu replaces what is on screen, and the parent stays where it was.
     *
     * The chosen item is shown three ways, so that it reads without colour alone: the
     * highlight behind it, its label in the active text and the chosen item's setting, and the
     * focus ring around it.
     *
     * Each row puts its label on the left and its value, as setting() gives it, on the right.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::menu::Menu>& menu) const;

    /**
     * Draw a menu bar - the strip across the top, and the open menu with any flyout the
     * cursor has descended into drawn under it. It takes the top of the canvas, and its
     * panels drop from there.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::menu::Bar>& bar) const;

    /**
     * Draw the shadow a box floating over the ui casts, moved down by the dressing's offset.
     * A menu's panel and a tooltip both cast one.
     **/
    void cast(v3d::render::realtime::canvas::Canvas* canvas, const glm::vec2& min, const glm::vec2& max) const;

 private:
    /**
     * The colour a menu item's label is written in, in a centred menu and in a dropped panel
     * alike. An item that cannot be used, or one in a menu that cannot be, is written in the
     * disabled colour even while it is the active one, because navigation does not activate it.
     **/
    static glm::vec4 itemInk(const boost::shared_ptr<component::menu::Item>& item, const Dressing& dress, bool selected);
    /**
     * What a centred menu's row shows on its right. That is an ellipsis while a capture waits
     * on it, the key bound to the command it binds, a slider's value between the arrows that
     * step it, or the value it holds. Empty when it shows none.
     *
     * @param root the menu the level belongs to, which holds the capture
     **/
    std::string setting(const component::menu::Item& item, const component::menu::Menu& root) const;

    /**
     * Draw one row of a centred menu in the box the arranger gave it: its highlight and ring
     * when it is chosen, its label on the left and its value on the right.
     *
     * @param span the left and right edges of the menu's panel
     **/
    void settingRow(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::menu::Item>& item,
        const glm::vec2& span, bool selected, const std::string& value) const;

    /**
     * The half side of the square that marks a checked menu item, as a fraction of a line.
     **/
    static constexpr float checkMarkScale = 0.15F;
    /**
     * The radius of the arrow that marks a submenu, as a fraction of a line.
     **/
    static constexpr float submenuArrowScale = 0.18F;

    /**
     * Draw one item of a dropped panel in the row the arranger gave it: its highlight, its mark
     * or icon, its submenu arrow, its label and its shortcut.
     *
     * @param panel the panel's box, which the row spans
     * @param keys the item's shortcut, or empty for none
     **/
    void entry(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::menu::Item>& item,
        const v3d::type::geometry::Bound2D& panel, bool selected, const std::string& keys) const;

    /**
     * Draw a separator: a rule across a menu at a height, between two edges.
     **/
    void rule(v3d::render::realtime::canvas::Canvas* canvas, float left, float right, float middle) const;

    /**
     * Draw one dropped panel of a menu bar, leaving every item holding its own row.
     *
     * @param origin where the panel's top left corner would go, before it is moved to
     *      keep it on the canvas
     **/
    void panel(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::menu::Menu>& menu,
        const glm::vec2& origin) const;

    /**
     * Draw a menu's panel, and everything in it, faded and lifted by how far it has opened.
     * Every begin() is matched by an end().
     **/
    void begin(v3d::render::realtime::canvas::Canvas* canvas, float open) const;
    static void end(v3d::render::realtime::canvas::Canvas* canvas);

    /**
     * The colours and metrics every menu is drawn with.
     **/
    const Dressing& base() const noexcept;

    Measure measure_;
    Write write_;
    const style::Resolver& styles_;
    const Arranger& arranger_;
    const Easing& easing_;
    Shortcut shortcut_;
    Emphasis emphasis_;
};

};  // namespace v3d::ui::paint::internal
