/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "Text.h"

#include "../type/Bound2D.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime {
class Canvas;
};  // namespace v3d::render::realtime

namespace v3d::ui {

class Component;
class Container;
class Engine;
class Style;

namespace style {
class Theme;
};  // namespace style

namespace component {
class Bar;
class Box;
class Button;
class CheckBox;
class Icon;
class Label;
class Menu;
class MenuBar;
class Panel;
class Scrollbar;
class SelectList;
class TabBar;
class Toolbar;
};  // namespace component

/**
 * Draws the ui onto a canvas of quads.
 *
 * A panel, a highlight and a line of text are all the batched quad of ADR-0005, so the
 * whole ui is added to whatever canvas the app is already filling and costs the frame no
 * pass and no draw of its own.
 *
 * Text is the caller's to lay out. This library knows where a label goes and how wide the
 * thing around it has to be; turning a string into glyph quads belongs to v3d::font and to
 * whatever atlas the app loaded, so those two are handed in as callbacks.
 *
 * Drawing is also what lays the ui out: every component is left holding the bounds it
 * was drawn in, which is what the cursor is tested against, per ADR-0019.
 *
 * Every component type this library has is drawn: menus, menu bars, toolbars, buttons,
 * labels, icons, panels, bars, scrollbars, check boxes, radio buttons, select lists, tab
 * bars and the two flow boxes.
 *
 * A component holds other components, and drawing one is what works out where they go:
 * every box is resolved against the box around it as the walk reaches it, per ADR-0034.
 * A flow box writes its children's boxes itself, because their positions are what it is
 * for.
 *
 * The strips stack in the order a container lists them. A menu bar takes the top of the
 * canvas, a top toolbar takes a band under whatever is already there, and a left toolbar
 * runs down the side of what is left. insets() is that arithmetic on its own, for an app
 * that has to know what area it is left with before anything has been drawn.
 *
 * A menu bar is nonetheless drawn last, because an open menu drops a panel over the
 * strips below it.
 **/
class ComponentRenderer {
 public:
    /**
     * What the ui cannot work out from the components alone: the colours and metrics a
     * component is drawn with.
     *
     * These are what a theme's "ui" style names, and what is left here is the default a
     * theme that names nothing draws in. theme() is what reads one in, per ADR-0020.
     *
     * Not a Style, which is the bag of properties a theme holds. This is what one resolves
     * to.
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

    /**
     * @param measure how wide a string is when the app draws it
     * @param write how the app draws a string
     **/
    ComponentRenderer(const Measure& measure, const Write& write);
    ~ComponentRenderer();

    /**
     * @return the colours and metrics the ui is drawn with, to be changed in place
     **/
    Dressing& dressing() noexcept;

    /**
     * Draw with a theme: read its "ui" style into dressing(), and keep it for the images a
     * button is drawn from.
     *
     * Every colour and metric the style does not name keeps the value it had, so a theme
     * carrying nothing changes nothing and a theme carrying one colour changes one.
     *
     * @param theme the active theme, or null to go back to drawing in the defaults
     **/
    void theme(const boost::shared_ptr<style::Theme>& theme);

    /**
     * @return the theme being drawn with, which is null until one is given
     **/
    boost::shared_ptr<style::Theme> theme() const noexcept;

    /**
     * Draw a label - one line of text at the position it holds.
     **/
    void draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Label>& label) const;

    /**
     * Draw an icon - the texture something resolved its source to, at the size the
     * component was given. An icon whose source was never resolved draws nothing.
     **/
    void draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Icon>& icon) const;

    /**
     * Draw a button at the position and size it holds, which is what a button in a
     * container carries and what a toolbar writes on the ones in the strip.
     **/
    void draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Button>& button) const;

    /**
     * Draw a panel - a filled box with a border, rounded by however much its style asks
     * for, at the box it holds.
     **/
    void draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Panel>& panel) const;

    /**
     * Draw a bar - the track it holds, and the fraction of it that is filled.
     **/
    void draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Bar>& bar) const;

    /**
     * Draw a scrollbar - its track, and the thumb over the part of the content its page
     * shows. A bar with nothing to scroll draws the track alone.
     **/
    void draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Scrollbar>& bar) const;

    /**
     * Draw a check box - the box, the mark when it is checked, and the label beside it.
     * A radio button is the same call: the mark is round and the style class is its own.
     **/
    void draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::CheckBox>& box) const;

    /**
     * Draw a select list - its plate, and as many of its rows as its box shows, with the
     * chosen one highlighted.
     *
     * The rows are cut off at the plate and moved up by what the list is scrolled by, per
     * ADR-0037, and the row height the style resolved to is left on the list so that it
     * can say which row a point is on.
     **/
    void draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::SelectList>& list) const;

    /**
     * Draw a tab bar - the strip of tabs across the top of its box, and the page the
     * chosen tab holds under it.
     *
     * Only the chosen page is drawn, so nothing in the others is laid out or picked, and
     * where each tab ended up is left on the bar for the cursor to be tested against.
     **/
    void draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::TabBar>& bar) const;

    /**
     * Draw every visible container of a ui engine.
     **/
    void draw(v3d::render::realtime::Canvas* canvas, const Engine& ui) const;

    /**
     * Draw every visible component of one container.
     **/
    void draw(v3d::render::realtime::Canvas* canvas, const Container& container) const;

    /**
     * Draw the active level of a menu, centred on the canvas.
     *
     * It is the active level rather than the menu itself that is drawn: descending into
     * a submenu replaces what is on screen, and the parent stays where it was.
     **/
    void draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Menu>& menu) const;

    /**
     * Draw a menu bar - the strip across the top, and the open menu with any flyout the
     * cursor has descended into drawn under it. It takes the top of the canvas, which is
     * where its panels are dropped from.
     **/
    void draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::MenuBar>& bar) const;

    /**
     * Draw a toolbar - a row of buttons across the canvas, or a column down its side.
     *
     * @param corner the strip's top left, which is where the edge it runs along has
     *      room for it once the strips before it have taken theirs
     **/
    void draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Toolbar>& bar,
        const glm::vec2& corner) const;

    /**
     * How much of the canvas the strips of one container cover - the band along the top
     * and the one down the left side, in pixels. What is left is what the app has to
     * draw in.
     *
     * @return the left inset in x and the top inset in y
     **/
    glm::vec2 insets(const Container& container) const;

    /**
     * The insets of every visible container of a ui engine.
     **/
    glm::vec2 insets(const Engine& ui) const;

 private:
    /**
     * Draw one component and everything it holds, into a box that has already been
     * resolved.
     *
     * @param bounds where this component goes, which its parent worked out
     **/
    void walk(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<Component>& component,
        const v3d::type::Bound2D& bounds) const;

    /**
     * Write the boxes of a flow box's children - along the line by what each asks for,
     * and across it by the box's width when it stretches them.
     *
     * @param bounds the box the children are laid out inside
     * @param boxes filled with one box per child, in the order the children are held
     **/
    void arrange(const component::Box& box, const v3d::type::Bound2D& bounds,
        std::vector<v3d::type::Bound2D>* boxes) const;

    /**
     * The size a component makes of itself, which is what an Auto extent resolves to -
     * the width of a label's text, the side of an icon, the room a button's label needs.
     * A component that decides nothing for itself asks for nothing.
     **/
    glm::vec2 natural(const Component& component) const;

    /**
     * Draw one dropped panel of a menu bar, leaving every item holding its own row.
     *
     * @param origin where the panel's top left corner would go, before it is moved to
     *      keep it on the canvas
     **/
    void panel(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Menu>& menu,
        const glm::vec2& origin) const;

    /**
     * How wide a toolbar's widest button is, which is what sizes a column and what a row
     * has no use for.
     **/
    float widest(const component::Toolbar& bar) const;

    /**
     * How much room one button asks for along a strip - its icon's side when it names
     * one, and otherwise its label's width.
     **/
    float extent(const component::Button& button) const;

    /**
     * Draw the nine images a button style names over the button's box - the four corners
     * at their own size, the four edges stretched along it, and the centre over the rest.
     *
     * @return false when the theme names no image for that button in that state, which is
     *      what leaves a flat button to be drawn instead
     **/
    bool skin(v3d::render::realtime::Canvas* canvas, const component::Button& button,
        const glm::vec2& min, const glm::vec2& max) const;

    /**
     * Find the style a component is drawn with.
     *
     * A component names a style; one that names none is drawn with whichever style of
     * that class the theme holds first, so that a theme can dress every button without
     * every button naming it.
     *
     * @param className the style class - "button", "ui"
     * @param name what the component's style() gives, which may be empty
     **/
    boost::shared_ptr<Style> lookup(const std::string& className, const std::string_view& name) const;

    Measure measure_;
    Write write_;
    Dressing dressing_;
    boost::shared_ptr<style::Theme> theme_;
};

};  // namespace v3d::ui
