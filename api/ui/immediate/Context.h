/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Immediate.h>
#include <api/ui/input/LineEdit.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/Text.h>
#include <api/ui/style/Style.h>

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::canvas {
class Canvas;
};  // namespace v3d::render::realtime::canvas

namespace v3d::ui::immediate {

/**
 * A widget's identity, as Immediate::Id.
 **/
typedef Immediate::Id Id;

/**
 * What a widget's box did with the cursor this frame.
 **/
struct Reaction final {
    Reaction() noexcept;

    bool hovered;  /**< the cursor is on it, and was on nothing over it last frame **/
    bool held;     /**< a press went down on it and has not come up **/
    bool clicked;  /**< a press and a release both landed on it **/
};

/**
 * The few bits a widget cannot work out again from the state it reads.
 **/
struct Retained final {
    Retained() noexcept;

    std::uint64_t frame;  /**< the last frame that asked for it, which it is aged out from **/
    unsigned int tab;  /**< which tab of a strip is selected **/
    float scroll;      /**< how far the content is scrolled up, in pixels **/
    float content;     /**< how tall what it held came to last frame **/
    glm::vec2 offset;  /**< how far a window has been dragged from where the caller put it **/
    bool collapsed;    /**< whether a window is folded to its title bar **/
    bool dragging;     /**< whether the press on its bar has travelled far enough to move it **/
    int chosen;        /**< the item a combo's list was clicked on, taken next frame, or -1 **/
};

/**
 * Where the next widget goes, and what the row being written has come to.
 **/
struct Row final {
    Row() noexcept;

    float margin;     /**< where a new row starts **/
    float right;      /**< where the room a widget may take ends **/
    float penY;       /**< where the next row starts **/
    float top;        /**< the top of the row being written **/
    float height;     /**< the tallest thing on it so far **/
    float lastRight;  /**< just past the last widget, which sameLine() starts from **/
    bool sameLine;
};

/**
 * The window being written, and the row state it interrupted.
 **/
struct Window final {
    Window() noexcept;

    bool open;           /**< whether a window is being written at all **/
    float margin;        /**< the row margin to go back to at endWindow() **/
    float right;         /**< and the right edge **/
    Id id;               /**< the id that holds the window's scroll and content **/
    Id scroll;           /**< the id its scrollbar is hit tested under **/
    glm::vec2 bodyMin;   /**< the part of it below the title bar, which is clipped to **/
    glm::vec2 bodyMax;
    float contentTop;    /**< where its content would start if it were not scrolled **/
    bool scrolls;
    bool clipped;
};

/**
 * The tab strip being written.
 **/
struct TabStrip final {
    TabStrip() noexcept;

    Id id;
    bool open;
    float pen;              /**< how far along the strip the next tab goes **/
    float top;
    unsigned int index;     /**< which tab of the strip this one is **/
    unsigned int wanted;    /**< the tab a click asked for, taken at endTabBar() **/
    bool changed;
    bool taken;             /**< whether the selected tab was drawn at all **/
};

/**
 * The table being written.
 **/
struct Table final {
    Table() noexcept;

    bool open;
    std::vector<std::string> headers;
    std::vector<float> widths;
    float left;             /**< where the row starts, which columns are measured from **/
    unsigned int column;    /**< which column the pen is in **/
    Id id;                  /**< the id that holds the table's scroll and content **/
    Id scroll;              /**< the id its scrollbar is hit tested under **/
    float right;            /**< the row right to go back to, and the region's right edge **/
    float height;           /**< how tall it is, or zero for a table that does not scroll **/
    float top;              /**< where it starts, which its height is measured from **/
    float contentTop;       /**< where its rows start, below the header **/
    bool clipped;
};

/**
 * The list an open combo shows, drawn over everything at end().
 **/
struct Popup final {
    Popup() noexcept;

    Id id;             /**< the combo that is open, or nothing **/
    bool asked;        /**< whether the combo was drawn this frame **/
    glm::vec2 boxMin;  /**< the combo's own box, where a press does not close the list **/
    glm::vec2 boxMax;
    std::vector<std::string> items;
    int selected;
};

/**
 * Whether a scrolled region puts up its scrollbar: a window decides at its start, from what
 * it drew last frame, and a table whenever there is more than it shows.
 **/
enum class Bar : std::uint8_t { Never, Always, WhenNeeded };

/**
 * A scrolled region. id holds its scroll and content, and scroll is the id its scrollbar is
 * hit tested under. contentTop is where its content would start if it were not scrolled.
 * bodyMax is the bottom right corner of its body, which the scrollbar runs down to.
 **/
struct Scrolled {
    Id id = 0;
    Id scroll = 0;
    float contentTop = 0.0F;
    glm::vec2 bodyMax{0.0F};
};

/**
 * Everything an Immediate holds, and the steps its widgets share.
 *
 * Immediate's header carries only what an app calls. This is the rest: what the layer keeps
 * between frames, what it tracks while a frame is written, and the layout, hit testing and
 * drawing every widget goes through. Each Immediate owns one.
 **/
class Context final {
 public:
    Context(const paint::Measure& measure, const paint::Write& write);

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
     * Draw a line of text with its box already worked out, vertically centred in it.
     **/
    void label(const std::string& line, const glm::vec2& min, const glm::vec2& size, const glm::vec4& colour) const;

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
     * Give the keyboard to a text field, or take it from whichever has it, and tell whoever
     * listens to onTyping().
     **/
    void focus(Id field);

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

    paint::Measure measure;
    paint::Write write;
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
    bool typingDrawn;     /**< whether that field has been drawn this frame **/
    std::string written;  /**< what the field last wrote to the caller's string **/
    input::LineEdit edit;
    input::Clipboard clipboard;
    std::function<void(bool)> onTyping;

    glm::vec2 itemMin;  /**< the box of the item placed last, which itemHovered() asks about **/
    glm::vec2 itemMax;
    Id itemOwner;       /**< what has to have held the cursor for that item to be hovered **/

    Popup popup;
};

};  // namespace v3d::ui::immediate
