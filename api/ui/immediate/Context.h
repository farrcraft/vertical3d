/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/ui/Clock.h>
#include <api/ui/Immediate.h>
#include <api/ui/immediate/Body.h>
#include <api/ui/immediate/Field.h>
#include <api/ui/immediate/Id.h>
#include <api/ui/immediate/Number.h>
#include <api/ui/immediate/Popup.h>
#include <api/ui/immediate/Reaction.h>
#include <api/ui/immediate/Retained.h>
#include <api/ui/immediate/Row.h>
#include <api/ui/immediate/Scrolled.h>
#include <api/ui/immediate/TabStrip.h>
#include <api/ui/immediate/Table.h>
#include <api/ui/immediate/Tip.h>
#include <api/ui/immediate/Window.h>
#include <api/ui/input/Clipboard.h>
#include <api/ui/input/LineEdit.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/style/Overlay.h>

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::canvas {
class Canvas;
};  // namespace v3d::render::realtime::canvas

namespace v3d::ui::paint {
class TextRenderer;
};  // namespace v3d::ui::paint

/**
 * What the immediate-mode layer keeps for its widgets between frames, such as windows, rows,
 * tables, tab strips and popups, and how a widget reacted to the cursor.
 **/
namespace v3d::ui::immediate {

/**
 * Everything an Immediate holds, and the steps its widgets share.
 *
 * Immediate's header carries only what an app calls. This is the rest: what the layer keeps
 * between frames, what it tracks while a frame is written, and the layout, hit testing and
 * drawing every widget goes through. Each Immediate owns one.
 **/
class Context final {
 public:
    /**
     * @param renderer what text is laid out and drawn through
     * @param size the size text is drawn at, or 0 for the renderer's own
     **/
    Context(const boost::shared_ptr<paint::TextRenderer>& renderer, float size);

    /**
     * How thick a separator and the line under a tab strip are.
     **/
    static constexpr float ruleWidth = 1.0F;

    /**
     * What a widget is holding, marked as still wanted so that end() does not age it out.
     **/
    Retained& retain(Id id);

    /**
     * Hash a label with the top of the id stack.
     **/
    Id identify(const std::string& label) const;

    /**
     * Work out where a widget of this size goes, and advance the pen past it.
     *
     * @return the widget's top left corner
     **/
    glm::vec2 place(const glm::vec2& size);

    /**
     * How wide the widget being written may be, and forget what asked for it.
     *
     * The rest of the row, unless nextItemWidth() set a width.
     **/
    float itemWidth();

    /**
     * Offer a box to the cursor.
     *
     * Two things have to agree for a widget to be hovered: the cursor is in its box now, and
     * nothing drawn over it took the cursor last frame. The first stops a widget reacting to
     * a release that landed somewhere else; the second lets a window drawn later take the
     * cursor from one under it.
     *
     * A disabled widget is offered nothing, which stops it lighting up as well as answering.
     **/
    Reaction interact(Id id, const glm::vec2& min, const glm::vec2& max);

    /**
     * @return how wide a line of text is at the layer's size, or 0 with no text renderer
     **/
    float measure(std::string_view line) const;

    /**
     * Draw a line of text with its box already worked out, vertically centred in it.
     **/
    void label(const std::string& line, const glm::vec2& min, const glm::vec2& size, const glm::vec4& colour) const;

    /**
     * Draw a title or a header as label() does, in the semibold face when the dressing's
     * titleWeight asks for it.
     **/
    void title(const std::string& line, const glm::vec2& min, const glm::vec2& size, const glm::vec4& colour) const;

    /**
     * @return the colour to draw text in, given what the caller asked for and whether
     *      anything has disabled it
     **/
    glm::vec4 ink(const glm::vec4& colour) const;

    /**
     * @return what to fill a widget's box with
     * @param lit whether it is held down or chosen, which read the same way
     * @param under whether the cursor is on it
     **/
    glm::vec4 face(bool lit, bool under) const;

    /**
     * Ease a widget's fill toward a colour over the dressing's hover duration, from wherever
     * its last change had reached. Without a clock the colour is drawn at once.
     *
     * @return the colour to draw this frame
     **/
    glm::vec4 eased(Id id, const glm::vec4& colour);

    /**
     * Give the keyboard to a text field, or take it from whichever has it, and tell whoever
     * listens to onTyping().
     **/
    void focus(Id field);

    /**
     * Give the keyboard to a text field, and start its edit from the caller's string.
     **/
    void claim(Id field, const std::string& value);

    /**
     * Start writing into the body of a window or a region: rows go inside it, clipped to it and
     * scrolled, and the wheel over it scrolls it.
     *
     * @param body the window or region the rows go into
     * @param shown false for a folded window, which takes the rows' margins and nothing else
     **/
    void openBody(const Body& body, bool shown);

    /**
     * Draw the line a field is editing between two corners, with the selection behind it and
     * the caret, slid left as far as the caret needs. The caller clips to the corners.
     **/
    void drawEdit(const glm::vec2& low, const glm::vec2& high);

    /**
     * A number field that has the keyboard: when it lets go, take what was typed, unless escape
     * let it go or the text is not a finite number.
     *
     * @return whether the value changed
     **/
    bool typeNumber(const Reaction& reaction, float* value);

    /**
     * A number field that does not have the keyboard: drag the value once a press has
     * travelled, finish the drag on release, and type into the field after a click.
     *
     * @return whether the value changed
     **/
    bool dragNumber(Id id, const Reaction& reaction, float* value, float speed);

    /**
     * Draw a number field: its label and value, or the line being typed into it.
     **/
    void drawNumber(const Number& field, const Reaction& reaction);

    /**
     * Draw a text field: its icon, its label, and its value, its placeholder or the line being
     * typed into it.
     **/
    void drawField(const Field& field, const Reaction& reaction);

    /**
     * Close a scrolled region, a window's body or a table: measure what was drawn into it,
     * draw its scrollbar down the right of the body, and turn the wheel over it into scroll.
     **/
    void closeScroll(const Scrolled& region, Bar bar);

    /**
     * Draw the bar down the right of a region that has more content than it shows, and
     * scroll it where the cursor drags the thumb to.
     *
     * A window and a table each have one, so the caller passes the rectangle. The bar is
     * drawn inside it.
     *
     * @param id the id the thumb is hit tested under
     * @param min the top left of the region the bar runs down
     * @param max its bottom right, which the bar is drawn against
     * @param view how much of the content the region shows, in pixels
     * @param span how much of it it does not, which is the furthest it can be scrolled
     * @param scroll read and written - where the region is scrolled to
     **/
    void scrollbar(Id id, const glm::vec2& min, const glm::vec2& max, float view, float span, float* scroll);

    /**
     * Draw the open combo's list over everything, and take a click on one of its items.
     **/
    void drawPopup();

    /**
     * Ask for a widget's tooltip, for the widget the cursor is on this frame.
     **/
    void askTip(Id id, const std::string& line, const std::string& keys, const glm::vec2& min, const glm::vec2& max);

    /**
     * Draw the tooltip asked for this frame, once the cursor has rested long enough, or forget
     * it when nothing asked.
     **/
    void drawTip();

    /**
     * Open a scrolling table's region, at the pen, the first time a row asks for one.
     *
     * Called from nextRow(), not from table(), because the region starts where the rows do:
     * below the header row when there is one, and at the top of the table when there is not.
     **/
    void tableBody();

    /**
     * @return how far along the row a column starts
     **/
    float columnStart(unsigned int index) const;

    // what text is laid out and drawn through, and the size it is drawn at
    boost::shared_ptr<paint::TextRenderer> text;
    float textSize;
    // what a theme's sizes are multiplied by
    float scale;
    // the time transitions are measured on, and whether one has been given
    Clock clock;
    bool timed;
    paint::Dressing dressing;
    // what the current theme wrote over dressing
    style::Overlay<paint::Dressing> overlay;

    v3d::render::realtime::canvas::Canvas* canvas;
    Immediate::Input input;
    glm::vec2 previousCursor;
    glm::vec2 drag;

    Id hovered;   /**< what the cursor was on last frame, which this frame uses **/
    Id hovering;  /**< what it is on this frame, which the next one will use **/
    Id active;    /**< what a press went down on **/
    glm::vec2 pressAt;  /**< and where it went down, which a travel is measured from **/
    float nextWidth;  /**< what the next widget was told to be, or nothing **/

    std::vector<Id> ids;
    std::map<Id, Retained> states;
    std::uint64_t frame;

    Row row;
    Window window;
    TabStrip tabs;
    Table table;
    unsigned int disabled;
    Id wheeled;  /**< the topmost window the cursor is over, which the wheel turns **/

    Id typing;            /**< the text field that holds the keyboard, or nothing **/
    bool letGo;           /**< a return, tab or escape asked the field to give it up **/
    bool escaped;         /**< and it was escape, which a number field takes as no change **/
    bool entered;         /**< and it was return **/
    bool focusNext;       /**< the next field drawn takes the keyboard **/
    bool typingDrawn;     /**< whether that field has been drawn this frame **/
    std::string written;  /**< what the field last wrote to the caller's string **/
    input::LineEdit edit;
    input::Clipboard clipboard;
    std::function<void(bool)> onTyping;

    glm::vec2 itemMin;  /**< the box of the item placed last, which itemHovered() asks about **/
    glm::vec2 itemMax;
    Id itemOwner;       /**< what has to have held the cursor for that item to be hovered **/
    bool itemFinished;  /**< whether an edit of that item ended this frame **/
    bool itemEntered;   /**< whether that edit ended with a return **/
    bool itemTyping;    /**< whether that item is a field that holds the keyboard **/

    Popup popup;
    Tip tip;

 private:
    /**
     * How far one notch of the wheel scrolls a window, as a multiple of a row. Three rows is
     * what a desktop scrolls by, and reads as a deliberate move rather than a nudge.
     **/
    static constexpr float wheelRows = 3.0F;
    /**
     * How far a press on a number has to travel across it before it drags the value instead of
     * typing into it, as a window's title bar decides between a move and a fold.
     **/
    static constexpr float dragThreshold = 3.0F;
    /**
     * How wide a number's edge is at a scale of 1, in pixels.
     **/
    static constexpr float edgeWidth = 2.0F;
    /**
     * How far a tooltip's text sits in from its edges, as a fraction of a line, as a component's
     * tooltip does.
     **/
    static constexpr float tipInset = 0.4F;
    /**
     * @return a typed number, or nothing when the text is not a whole finite number
     **/
    static std::optional<float> number(std::string_view text);
    /**
     * The seed and the multiplier of FNV-1a, which hashes a label into an id.
     * Any hash would do; this one is four lines and needs no table.
     **/
    static constexpr std::uint64_t hashSeed = 1469598103934665603ULL;
    static constexpr std::uint64_t hashPrime = 1099511628211ULL;
};

};  // namespace v3d::ui::immediate
