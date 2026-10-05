/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/geometry/Bound2D.h>
#include <api/ui/paint/Text.h>

#include <functional>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::render::realtime {
class Canvas;
};  // namespace v3d::render::realtime

namespace v3d::ui {

class Component;
class Container;

namespace style {
class Resolver;
};  // namespace style

namespace component {
class Box;
class Button;
class Menu;
class MenuBar;
class TabBar;
class Toolbar;
};  // namespace component

/**
 * Works out where every component of a tree goes, and leaves each holding the box it was
 * put in.
 *
 * **One pass decides both what is drawn and what is clicked**, so a hit box cannot drift
 * from the thing it belongs to. The layout pass is here and the drawing is
 * ComponentRenderer's, joined by a callback, so a box can be resolved without a canvas and
 * the drawing side holds no layout arithmetic.
 *
 * Nothing here is retained. A component's position() and size() are the output, written as
 * the pass reaches it, so nothing has a box until the pass has reached it.
 **/
class Arranger final {
 public:
    /**
     * Draw one component, whose box has just been written onto it.
     *
     * The hook between resolving a box and filling it. A callback rather than a base class,
     * as with text, so the layout pass names no renderer and a test can watch it place a
     * tree without drawing one.
     **/
    typedef std::function<void(v3d::render::realtime::Canvas*, const boost::shared_ptr<Component>&)> Paint;

    /**
     * How thick the rule along a strip's far edge is, and the gap it puts between one
     * strip and the next.
     **/
    static const float ruleWidth;

    /**
     * @param measure how wide a string is when the app draws it, which sizes a
     *      label, a button and a tab
     * @param styles where a component's colours and metrics come from - held by reference
     *      because the renderer owns it and a theme change has to reach here
     **/
    Arranger(const paint::Measure& measure, const style::Resolver& styles);

    /**
     * Lay a component out inside a box that has already been resolved, and draw it and
     * everything it holds.
     *
     * The canvas and the paint are both optional. Given neither, this resolves every box
     * and draws nothing, which is how to ask for layout on its own.
     *
     * @param bounds where this component goes, which its parent worked out
     * @param paint what fills each box as it is written, or empty to place and not draw
     **/
    void walk(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<Component>& component,
        const v3d::type::geometry::Bound2D& bounds, const Paint& paint) const;

    /**
     * The size a component makes of itself, which an Auto extent resolves to - the
     * width of a label's text, the side of an icon, the room a button's label needs.
     *
     * A component that decides nothing for itself takes the room it was offered. Nothing
     * here reads a box an earlier pass wrote.
     *
     * Takes the component to write on rather than to read: a list is left holding how wide
     * its widest row measured, the way layout leaves every component holding its box.
     *
     * @param room what the component is being offered. A flow box offers no room along the
     *      line it lays out, because the line is shared, so an Auto extent there is what
     *      the component makes of itself and nothing more
     **/
    glm::vec2 natural(Component& component, const v3d::type::geometry::Bound2D& room) const;

    /**
     * Where each strip of a container goes, and how much of the canvas they take between
     * them.
     *
     * They stack in the order the container draws them: a menu bar takes the top, a top
     * toolbar takes a band under whatever is already there, and a left toolbar runs down
     * the side of what is left.
     *
     * Both the draw and insets() use this, so the area an app is told it has always
     * matches where the strips are drawn.
     *
     * @param strips filled with one toolbar and its top left corner per strip, in the
     *      order they are drawn; null when only the total is wanted
     * @param bars filled with the container's menu bars, which are drawn last because an
     *      open menu drops a panel over the strips below it; null when only the total is
     *      wanted
     * @return the left inset in x and the top inset in y, in pixels
     **/
    glm::vec2 stack(const Container& container,
        std::vector<std::pair<boost::shared_ptr<component::Toolbar>, glm::vec2>>* strips,
        std::vector<boost::shared_ptr<component::MenuBar>>* bars) const;

    /**
     * Where a tab bar's chosen page goes - the room the strip and its rule leave under
     * them.
     **/
    v3d::type::geometry::Bound2D page(const component::TabBar& bar) const;

    /**
     * How much room one button asks for along a strip - its icon's side when it names one,
     * and otherwise its label's width.
     **/
    float extent(const component::Button& button) const;

    /**
     * How wide a toolbar's widest button is, which sizes a column. A row does not use it.
     **/
    float widest(const component::Toolbar& bar) const;

    /**
     * Leave a component holding the bounds it was put in, which the cursor is tested
     * against. The only way a box is given, in layout alone and in a draw.
     *
     * Through a reference to the base, because a menu's own size() is its item count and
     * hides the one that means how big it is.
     **/
    static void place(Component& component, const glm::vec2& position, const glm::vec2& size);

    /**
     * The size a component is drawn at: the box layout gave it, or its natural size when it
     * was drawn without one.
     **/
    glm::vec2 drawn(Component& component) const;

    /**
     * Place a toolbar and every button on it. A row spans the room from its corner and is a
     * strip high; a column is as wide as its widest button and runs the height of the room.
     *
     * @param room the extent of what the strip is drawn into, the canvas
     **/
    void strip(component::Toolbar& bar, const glm::vec2& corner, const glm::vec2& room) const;

    /**
     * Place a menu bar's open panel and its items, hanging from an origin and moved back into
     * the room where it would hang off an edge.
     **/
    void panel(component::Menu& menu, const glm::vec2& origin, const glm::vec2& room) const;

    /**
     * Place a game menu's level and its items, centred in the room.
     **/
    void centred(component::Menu& level, const glm::vec2& room) const;

    /**
     * How wide a flyout panel's mark column is, as a fraction of a line - the room on each
     * side of its labels for a check mark and a submenu arrow.
     **/
    static constexpr float markColumn = 0.9f;

 private:
    /**
     * The room a flow box offers each child: none along the line it lays out, the whole of
     * it across.
     **/
    static v3d::type::geometry::Bound2D lineRoom(bool vertical, const v3d::type::geometry::Bound2D& bounds);

    /**
     * The size a child of a flow box asks for in the room it is offered, resolved against the
     * box's extent.
     **/
    glm::vec2 childSize(Component& child, const v3d::type::geometry::Bound2D& room, const glm::vec2& extent) const;

    /**
     * Write the boxes of a flow box's children - along the line by what each asks for, and
     * across it by the box's width when it stretches them.
     *
     * @param bounds the box the children are laid out inside
     * @param boxes filled with one box per child, in the order the children are held
     **/
    void arrange(const component::Box& box, const v3d::type::geometry::Bound2D& bounds,
        std::vector<v3d::type::geometry::Bound2D>* boxes) const;

    /**
     * The natural size of a flow box: the room it is offered, or for one that wraps, the line
     * it is given and the depth of the lines its children come to.
     **/
    glm::vec2 box(const Component& component, const v3d::type::geometry::Bound2D& room) const;

    /**
     * The same for a box that wraps: along a line until the next child would pass its end,
     * then on a new line. A child longer than the line has a line of its own.
     *
     * @return how far across its lines the children reach, which is the box's natural size
     *         in that direction
     **/
    float wrapped(const component::Box& box, const v3d::type::geometry::Bound2D& bounds,
        std::vector<v3d::type::geometry::Bound2D>* boxes) const;

    paint::Measure measure_;
    const style::Resolver& styles_;
};

};  // namespace v3d::ui
