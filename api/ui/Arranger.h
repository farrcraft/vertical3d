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
class MenuBar;
class TabBar;
class Toolbar;
};  // namespace component

/**
 * Works out where every component of a tree goes, and leaves each holding the box it was
 * put in.
 *
 * This is one half of ADR-0019's rule and ADR-0034's box model: **one walk decides both
 * what is drawn and what is clicked**, which is what stops a hit box drifting from the
 * thing it belongs to. The walk is here and the drawing is ComponentRenderer's, joined by
 * a callback rather than by one class doing both - so a box can be resolved without a
 * canvas, and the drawing side names no layout arithmetic.
 *
 * Nothing here is retained. A component's position() and size() are the output, written as
 * the walk reaches it, so nothing has a box until it has been walked.
 **/
class Arranger final {
 public:
    /**
     * Draw one component, whose box has just been written onto it.
     *
     * The seam between resolving a box and filling it, in the shape this library already
     * takes text: a callback rather than a base class, so that the walk names no renderer
     * and a test can watch it place a tree without drawing one.
     **/
    typedef std::function<void(v3d::render::realtime::Canvas*, const boost::shared_ptr<Component>&)> Paint;

    /**
     * How thick the rule along a strip's far edge is, and the gap it puts between one
     * strip and the next.
     **/
    static const float ruleWidth;

    /**
     * @param measure how wide a string is when the app draws it, which is what sizes a
     *      label, a button and a tab
     * @param styles where a component's colours and metrics come from - held by reference
     *      because the renderer owns it and a theme change has to reach here
     **/
    Arranger(const paint::Measure& measure, const style::Resolver& styles);

    /**
     * Lay a component out inside a box that has already been resolved, and draw it and
     * everything it holds.
     *
     * The canvas and the paint are both optional: a walk given neither resolves every box
     * and draws nothing, which is how layout is asked for on its own. ADR-0034 recorded
     * having to draw a tree to find out where it went as the cost of putting layout in the
     * draw walk; it is the walk that layout lives in rather than the drawing.
     *
     * @param bounds where this component goes, which its parent worked out
     * @param paint what fills each box as it is written, or empty to place and not draw
     **/
    void walk(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<Component>& component,
        const v3d::type::geometry::Bound2D& bounds, const Paint& paint) const;

    /**
     * The size a component makes of itself, which is what an Auto extent resolves to - the
     * width of a label's text, the side of an icon, the room a button's label needs.
     *
     * A component that decides nothing for itself takes the room it was offered. Nothing
     * here is answered from a box an earlier walk wrote, per ADR-0039.
     *
     * Takes the component to write on rather than to read: a list is left holding how wide
     * its widest row measured, the way the walk leaves every component holding its box.
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
     * One implementation, because the draw places the strips and insets() tells an app
     * what area is left around them. The two disagreeing means a ui drawn over the room an
     * app was told it had.
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
     * How wide a toolbar's widest button is, which is what sizes a column and what a row
     * has no use for.
     **/
    float widest(const component::Toolbar& bar) const;

 private:
    /**
     * Write the boxes of a flow box's children - along the line by what each asks for, and
     * across it by the box's width when it stretches them.
     *
     * @param bounds the box the children are laid out inside
     * @param boxes filled with one box per child, in the order the children are held
     **/
    void arrange(const component::Box& box, const v3d::type::geometry::Bound2D& bounds,
        std::vector<v3d::type::geometry::Bound2D>* boxes) const;

    paint::Measure measure_;
    const style::Resolver& styles_;
};

};  // namespace v3d::ui
