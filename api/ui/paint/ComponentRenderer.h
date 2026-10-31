/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>
#include <api/type/geometry/Bound2D.h>
#include <api/ui/Arranger.h>
#include <api/ui/Transition.h>
#include <api/ui/style/Resolver.h>

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Dressing.h"
#include "Text.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::canvas {
class Canvas;
};  // namespace v3d::render::realtime::canvas

namespace v3d::ui {

class Component;
class Container;
class Interface;

namespace style {
class Style;
class Theme;
};  // namespace style

namespace component {
class Bar;
class Box;
class Button;
class CheckBox;
class Icon;
class Label;
class Panel;
class Scrollbar;
class Slider;
class SelectList;
class TabBar;
class TextBox;
class Toolbar;
};  // namespace component

namespace component::menu {
class Bar;
class Item;
class Menu;
};  // namespace component::menu

};  // namespace v3d::ui

namespace v3d::ui::paint {

/**
 * Draws the ui onto a canvas of quads.
 *
 * A panel, a highlight and a line of text are all batched quads. The whole ui is added to
 * whatever canvas the app is already filling, and costs the frame no pass and no draw of
 * its own.
 *
 * This library decides where a label goes and how wide the component around it is. Turning
 * a string into glyph quads belongs to v3d::font and to whatever atlas the app loaded, so
 * measuring and writing text are passed in as callbacks.
 *
 * Drawing also lays the ui out. Every component is left holding the bounds it was drawn
 * in, and the cursor is tested against those bounds, so nothing is hit before it has been
 * drawn.
 *
 * Drawing a component also places the components it holds. Each box is resolved against
 * its parent's box as the layout pass reaches it. A flow box works out its children's boxes
 * itself.
 *
 * Every menu bar takes a band at the top of the canvas. Each top toolbar takes a band under
 * those, in the order the container lists them, and each left toolbar runs down the side
 * below the bands. insets() does that arithmetic on its own, for an app that needs to know
 * its remaining area before anything has been drawn.
 *
 * A menu bar is nonetheless drawn last, because an open menu drops a panel over the
 * strips below it.
 **/
class ComponentRenderer {
 public:
    /**
     * @param measure how wide a string is when the app draws it
     * @param write how the app draws a string
     **/
    ComponentRenderer(const Measure& measure, const Write& write);
    ~ComponentRenderer();

    /**
     * @return the colours and metrics the ui is drawn with, to be changed in place.
     *      Changing them drops what the resolver has worked out from them, so an app
     *      that sets its metrics once at startup pays for that once
     **/
    Dressing& dressing() noexcept;

    /**
     * @return the colours and metrics the ui is drawn with, read without changing them, which
     *      keeps what the resolver has worked out
     **/
    const Dressing& dressing() const noexcept;

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
     * Multiply a theme's sizes, which are logical pixels. dressing() is not changed, so the
     * caller sets it at the same scale. Takes effect at the next theme().
     **/
    void scale(float factor) noexcept;

    /**
     * The time the next draw is at. A change of a fill or a text colour eases over the
     * dressing's hover duration, and a menu opens over its open duration, both measured on
     * this clock. A renderer that is never given a clock draws every change at once.
     **/
    void clock(const Clock& now) noexcept;

    /**
     * How a command's shortcut is shown, such as "Ctrl+Z", or empty when nothing binds it.
     **/
    typedef std::function<std::string(const v3d::event::Event& command)> Shortcut;

    /**
     * Look up the shortcut a menu item shows beside its label, and a tooltip after its text.
     * Without one, nothing shows a shortcut.
     **/
    void shortcuts(const Shortcut& lookup);

    /**
     * @return the shortcut of the command a component sends, or empty when it sends none or
     *         nothing binds it
     **/
    std::string shortcut(const v3d::event::Event& command) const;

    /**
     * Draw a tooltip: a plate holding a line of text, and the shortcut after it in secondary
     * text, with a shadow under it. It hangs just below a box, or beside it, as wide as it needs,
     * and is moved back onto the canvas where it would leave it. Draw it last, over everything
     * else.
     *
     * @param below the box it hangs by, which is the component it describes
     * @param beside whether it hangs to the right of the box, centred on it, rather than under
     *        it. A tooltip that would leave the canvas there goes to the left instead
     **/
    void tooltip(v3d::render::realtime::canvas::Canvas* canvas, const std::string& text, const std::string& keys,
        const v3d::type::geometry::Bound2D& below, bool beside = false) const;

    /**
     * Draw a label - one line of text at the position it holds.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::Label>& label) const;

    /**
     * Draw an icon - the texture something resolved its source to, at the size the
     * component was given. An icon whose source was never resolved draws nothing.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::Icon>& icon) const;

    /**
     * Draw a button at the position and size it holds. The layout pass writes these for a
     * button in a container, and the toolbar's layout writes them for a button on a toolbar.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::Button>& button) const;

    /**
     * Draw a panel: a filled box with a border, with the corner radius its style sets, at
     * the box it holds.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::Panel>& panel) const;

    /**
     * Draw a bar - the track it holds, and the fraction of it that is filled.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::Bar>& bar) const;

    /**
     * Draw a scrollbar - its track, and the thumb over the part of the content its page
     * shows. A bar with nothing to scroll draws the track alone.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::Scrollbar>& bar) const;

    /**
     * A slider's track, the fill up to its value, and its thumb.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::Slider>& slider) const;

    /**
     * Draw a check box - the box, the mark when it is checked, and the label beside it.
     * A radio button is the same call: the mark is round and the style class is its own.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::CheckBox>& box) const;

    /**
     * Draw a select list: its background, and as many of its rows as its box shows, with
     * the chosen one highlighted.
     *
     * The rows are clipped to the background and moved up by the scroll offset. The row
     * height from the style is stored on the list, so that SelectList::at() can find the row
     * under a point.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::SelectList>& list) const;

    /**
     * Draw a text box: its background, the line it holds, and the caret when it has the
     * keyboard.
     *
     * The line is clipped to the background, and slid left when the caret would be past the
     * far edge, so a full box can still be typed into. The caret is drawn when the
     * component's focused() flag, which Interface::focus() sets, is true and the box is usable.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::TextBox>& box) const;

    /**
     * Draw a tab bar: the strip of tabs across the top of its box, and the background of
     * the chosen page under it. The layout pass draws what the page holds.
     *
     * Only the chosen page is drawn, so nothing in the others is laid out or picked. Each
     * tab's box is stored on the bar for the cursor to be tested against.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::TabBar>& bar) const;

    /**
     * Draw every visible container of a ui::Interface.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const Interface& ui) const;

    /**
     * Draw every visible component of one container.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const Container& container) const;

    /**
     * Draw the active level of a menu, centred on the canvas.
     *
     * It is the active level rather than the menu itself that is drawn: descending into
     * a submenu replaces what is on screen, and the parent stays where it was.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::menu::Menu>& menu) const;

    /**
     * Draw a menu bar - the strip across the top, and the open menu with any flyout the
     * cursor has descended into drawn under it. It takes the top of the canvas, and its
     * panels drop from there.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::menu::Bar>& bar) const;

    /**
     * Draw a toolbar - a row of buttons across the canvas, or a column down its side.
     *
     * @param corner the strip's top left, past the room the strips before it have taken
     *      along the same edge
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::Toolbar>& bar,
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
     * The insets of every visible container of a ui::Interface.
     **/
    glm::vec2 insets(const Interface& ui) const;

 private:
    /**
     * The colour a component's own text is drawn in: the given colour, or the theme's disabled
     * colour when the component, or anything holding it, cannot be used. An icon is tinted with
     * the same colour, so disabling a box greys both a label and the picture beside it.
     **/
    static glm::vec4 ink(const Component& component, const Dressing& dress, const glm::vec4& colour);
    /**
     * The colour a menu item's label is written in, in a centred menu and in a dropped panel
     * alike. An item that cannot be used, or one in a menu that cannot be, is written in the
     * disabled colour even while it is the active one, because navigation does not activate it.
     **/
    static glm::vec4 itemInk(const boost::shared_ptr<component::menu::Item>& item, const Dressing& dress, bool selected);
    /**
     * How many segments a radio button's disc is drawn with. Small enough to cost little at
     * the size a mark is drawn, large enough that the rim does not read as a polygon.
     **/
    static constexpr unsigned int markSides = 16;
    /**
     * How much of the box, or of the disc, a mark fills. The rest is the gap that makes it
     * read as a mark inside something rather than as a filled box.
     **/
    static constexpr float markFill = 0.5F;
    /**
     * How far in from the edge of a skinned button the corner images reach, when the
     * style names no corner of its own. A texture carries no size a handle can be asked
     * for, so this is a number rather than something measured.
     **/
    static constexpr float defaultCorner = 8.0F;
    /**
     * How far down a widget the baseline of its text sits, as a fraction of the widget's height.
     **/
    static constexpr float widgetBaseline = 0.7F;
    /**
     * How far down a line of text its baseline sits, as a fraction of the line height. The rest
     * is room for descenders.
     **/
    static constexpr float lineBaseline = 0.75F;
    /**
     * The half side of the square that marks a checked menu item, as a fraction of a line.
     **/
    static constexpr float checkMarkScale = 0.15F;
    /**
     * The radius of the arrow that marks a submenu, as a fraction of a line.
     **/
    static constexpr float submenuArrowScale = 0.18F;
    /**
     * How far a tooltip's text sits in from its edges, as a fraction of a line.
     **/
    static constexpr float tooltipInset = 0.4F;

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
     * Draw the shadow a menu's box casts, moved down by the dressing's offset.
     **/
    void cast(v3d::render::realtime::canvas::Canvas* canvas, const glm::vec2& min, const glm::vec2& max) const;

    /**
     * Draw one dropped panel of a menu bar, leaving every item holding its own row.
     *
     * @param origin where the panel's top left corner would go, before it is moved to
     *      keep it on the canvas
     **/
    void panel(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::menu::Menu>& menu,
        const glm::vec2& origin) const;

    /**
     * Draw one component into the box the layout pass has just written onto it. This is
     * the switch on component type that the Arranger calls back.
     **/
    void paint(v3d::render::realtime::canvas::Canvas* canvas,
        const boost::shared_ptr<Component>& component) const;

    /**
     * Trace the ring that says where the keyboard is, over a component that has just been
     * drawn. Nothing is traced for a component that is not focused.
     **/
    void ring(v3d::render::realtime::canvas::Canvas* canvas,
        const boost::shared_ptr<Component>& component) const;

    /**
     * Draw the nine images a button style names over the button's box - the four corners
     * at their own size, the four edges stretched along it, and the centre over the rest.
     *
     * @return false when the theme names no image for that button in that state, and a
     *      flat button is drawn instead
     **/
    bool skin(v3d::render::realtime::canvas::Canvas* canvas, const component::Button& button,
        const glm::vec2& min, const glm::vec2& max) const;

    /**
     * The colours and metrics before any component's own style class is applied over
     * them. The parts of the ui with no style class of their own are drawn with these:
     * a menu panel, a toolbar strip, a label.
     **/
    const Dressing& base() const noexcept;

    /**
     * Ease a colour toward what is asked for, or fade it out in its own colour when nothing
     * is, over the hover duration.
     **/
    glm::vec4 toward(Transition* transition, const glm::vec4& target) const;
    glm::vec4 away(Transition* transition) const;

    /**
     * How far open something is, from 0 to 1, easing out so it slows as it lands.
     **/
    float opened(Opening* opening) const;

    /**
     * Draw a menu's panel, and everything in it, faded and lifted by how far it has opened.
     * Every begin() is matched by an end().
     **/
    void begin(v3d::render::realtime::canvas::Canvas* canvas, float open) const;
    static void end(v3d::render::realtime::canvas::Canvas* canvas);

    Measure measure_;
    Write write_;
    style::Resolver styles_;
    // the layout pass: it resolves the boxes and calls back here to fill them
    Arranger arranger_;
    Clock clock_;
    Shortcut shortcut_;
    // whether clock() has been called, without which nothing eases
    bool timed_;
};

};  // namespace v3d::ui::paint
