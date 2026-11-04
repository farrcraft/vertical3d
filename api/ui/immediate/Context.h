/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Image.h>
#include <api/ui/Immediate.h>
#include <api/ui/Transition.h>
#include <api/ui/input/LineEdit.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/style/Style.h>

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
    unsigned int tabs; /**< how many tabs the strip held last frame, which share its width out **/
    float scroll;      /**< how far the content is scrolled up, in pixels **/
    float content;     /**< how tall what it held came to last frame **/
    glm::vec2 offset;  /**< how far a window has been dragged from where the caller put it **/
    bool collapsed;    /**< whether a window is folded to its title bar **/
    bool dragging;     /**< whether the press on its bar has travelled far enough to move it **/
    int chosen;        /**< the item a combo's list was clicked on, taken next frame, or -1 **/
    Transition face;   /**< the fill of its box, easing from one colour to the next **/
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
    float share;            /**< how wide each tab is, or 0 for as wide as its label **/
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
 * The body of a window or a region: the id that holds its scroll and content, the id its
 * scrollbar is hit tested under, its box, and where the part below any title bar starts.
 **/
struct Body {
    Id id = 0;
    Id scroll = 0;
    glm::vec2 min{0.0F};
    glm::vec2 max{0.0F};
    float top = 0.0F;
};

/**
 * A text field being drawn: its id, its label, the value it shows, its box, and what leads it.
 **/
struct Field {
    Id id = 0;
    std::string label;
    const std::string* value = nullptr;
    glm::vec2 min{0.0F};
    glm::vec2 max{0.0F};
    Image icon;                // drawn at the left, or nothing when it is not valid
    float iconSide = 0.0F;     // how big the icon is drawn
    float start = 0.0F;        // where the label starts, past the icon
    bool labelled = false;     // the label is drawn after the icon, and not as a placeholder
    glm::vec2 low{0.0F};       // where the text starts, past both
    glm::vec2 high{0.0F};      // where the text ends
};

/**
 * A number field being drawn: its id, its label, the value it shows, and its box.
 **/
struct Number {
    Id id = 0;
    std::string label;
    float value = 0.0F;
    glm::vec2 min{0.0F};
    glm::vec2 max{0.0F};
    std::optional<glm::vec4> labelColour;  // in place of secondary text
    std::string unit;                      // written straight after the value
    std::optional<glm::vec4> edge;         // a rule down the left side
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
