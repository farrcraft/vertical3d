/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/geometry/Bound2D.h>
#include <api/ui/input/LineEdit.h>
#include <api/ui/paint/Text.h>
#include <api/ui/style/Style.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::canvas {
class Canvas;
};  // namespace v3d::render::realtime::canvas

namespace v3d::ui {

namespace style {
class Theme;
};  // namespace style

/**
 * A ui written as calls rather than as a tree.
 *
 * A panel is a sequence between begin() and end(). Each widget is placed where the layout
 * pen has got to and drawn onto the same quad canvas the retained components draw onto. It
 * is hit tested against the box it was just drawn in, and returns its result on the spot.
 * A widget is drawn from the state it shows each frame, so nothing has to be kept in step
 * with that state. That suits tool panels that read live state.
 *
 * Text is drawn through callbacks, as with ComponentRenderer. This class decides how wide a
 * string is and where it goes, and v3d::font turns it into glyphs.
 *
 * Which widget the cursor is on is decided as a frame is drawn and used by the next one,
 * so a widget drawn later takes the cursor from one under it. A widget that moved is
 * therefore hovered a frame late.
 *
 * A window clips what it holds to its own edges, through the canvas's clip rectangle, and
 * scrolls it. The content height is measured as it is drawn, so a window decides whether
 * it needs a scrollbar from what the frame before it held.
 *
 * Keys reach a text field as they arrive, through press() and typed(), between one frame
 * and the next. The field that holds the keyboard edits a line the layer keeps, and writes
 * it to the caller's string when it is next drawn.
 **/
class Immediate {
 public:
    /**
     * A widget's identity: its label hashed with whatever pushId() has put on the stack. Two
     * widgets with the same label in the same scope are one widget to the cursor.
     **/
    typedef std::uint64_t Id;

    /**
     * What the cursor did since the last frame. An app fills this from its input devices.
     * pressed and released are the edges of down, and each is true for one frame.
     **/
    struct Input final {
        Input() noexcept;

        glm::vec2 cursor;
        bool down;
        bool pressed;
        bool released;
        float wheel;  /**< notches turned since the last frame, away from the reader first **/
    };

    /**
     * The colours and metrics the immediate ui draws with. A value that a theme's "tools"
     * style does not set keeps its default, so an empty theme changes nothing.
     *
     * A Style is the set of properties a theme holds. This struct is what one resolves to.
     **/
    struct Dressing final {
        Dressing() noexcept;

        float lineHeight;      /**< the height of one row of text **/
        float padding;         /**< the gap between a widget's edge and the text in it **/
        float spacing;         /**< the gap between one widget and the next **/
        float barHeight;       /**< how tall a title bar, a tab or a button is **/
        float borderWidth;     /**< how thick a window's outline is **/
        float radius;          /**< how far a window's corners are rounded **/
        float indent;          /**< how far a bullet pushes its text in **/
        float scrollbarWidth;  /**< how wide the bar down a window that scrolls is **/
        glm::vec4 panel;       /**< a window's background **/
        glm::vec4 border;      /**< a window's outline **/
        glm::vec4 titleBar;    /**< the band across the top of a window, and a table's header **/
        glm::vec4 text;        /**< ordinary text **/
        glm::vec4 activeText;  /**< text on a hovered or selected widget **/
        glm::vec4 dimText;     /**< text that is disabled or secondary **/
        glm::vec4 widget;      /**< a button, a track, a tab that is not selected **/
        glm::vec4 highlight;   /**< a selected row, a selected tab **/
        glm::vec4 hover;       /**< what is drawn behind the widget the cursor is on **/
        glm::vec4 fill;        /**< the filled part of a progress bar **/
        glm::vec4 rule;        /**< a separator, and the line under a tab strip **/
    };

    /**
     * @param measure how wide a string is when the app draws it
     * @param write how the app draws a string
     **/
    Immediate(const paint::Measure& measure, const paint::Write& write);
    ~Immediate();

    /**
     * Replace how strings are measured and drawn, as when the text is rescaled. Everything
     * the layer remembers between frames - where windows were dragged, what is folded, how
     * far a list is scrolled - is kept.
     **/
    void text(const paint::Measure& measure, const paint::Write& write);

    /**
     * @return the colours and metrics, to be changed in place
     **/
    Dressing& dressing() noexcept;

    /**
     * Read a theme's "tools" style into dressing().
     *
     * A separate class from the "ui" class the retained components read, because a tool
     * panel uses the same keys at smaller sizes than a hud. A theme styles both, in two
     * classes.
     **/
    void theme(const boost::shared_ptr<style::Theme>& theme);

    /**
     * Start a frame. Everything drawn until end() goes onto this canvas.
     **/
    void begin(v3d::render::realtime::canvas::Canvas* canvas, const Input& input);

    /**
     * Finish a frame: settle who has the cursor for the next one, and age out what nothing
     * has drawn for retention frames.
     **/
    void end();

    /**
     * How many widgets the immediate ui holds state for.
     *
     * It keeps only a window's scroll and fold, a table's scroll and a tab strip's
     * selection, so this is a handful in a running app. A caller that builds ids out of
     * changing text can watch it to see the cost.
     **/
    std::size_t retained() const noexcept;

    /**
     * How many frames a widget that stops being drawn keeps what it was holding.
     *
     * Long enough that a panel behind a toggle comes back scrolled and folded as it was,
     * and short enough that a caller building ids out of changing text is bounded by what
     * it drew in the last second rather than by how long it has been running.
     **/
    static const std::uint64_t retention = 60;

    /**
     * Open a window at a place the caller decides. Dragging the title bar moves it from
     * there.
     *
     * The position is an anchor. A drag is kept in the window's retained state as a
     * displacement from it, so a window the caller repositions every frame follows and
     * keeps the offset it was dragged by. The displacement is clamped so the title bar
     * stays on the canvas. A press on the bar that stays within a few pixels folds the
     * window on release; one that travels past that drags it and does not fold it.
     *
     * What goes in it is clipped to the window's edges and scrolls when there is more of
     * it than fits. The content height is last frame's, so the bar appears on the frame
     * after the one that overflowed, and a window whose content changes every frame sizes
     * its thumb a frame behind.
     *
     * Pair every call with endWindow(), whatever it returned. A collapsed window returns
     * false and still has to be closed.
     *
     * @param area where the window's top left corner is, and its size
     * @param alpha what the window's background is drawn at, 1 for opaque
     * @return whether what goes in it should be drawn, false for a collapsed window
     **/
    bool window(const std::string& title, const v3d::type::geometry::Bound2D::Area& area, float alpha);
    void endWindow();

    /**
     * One line of text at the pen.
     **/
    void text(const std::string& line);

    /**
     * A line of text in the dim colour, for secondary text.
     **/
    void textDisabled(const std::string& line);

    /**
     * Text broken at the spaces that do not fit the width the window leaves.
     **/
    void textWrapped(const std::string& line);

    /**
     * A line of text behind a bullet, indented past it.
     **/
    void bulletText(const std::string& line);

    /**
     * @return whether the button was clicked this frame, which is a press and a release
     *      that both landed on it
     **/
    bool button(const std::string& label);

    /**
     * A button the height of a line rather than of a bar, for one that sits in a row of
     * text.
     **/
    bool smallButton(const std::string& label);

    /**
     * A row that spans the width, drawn highlighted when it is the selected one.
     * @return whether it was clicked
     **/
    bool selectable(const std::string& label, bool selected);

    /**
     * An integer scrubbed by dragging across it.
     *
     * @param value read and written; left alone when nothing dragged
     * @return whether the value changed this frame
     **/
    bool dragInt(const std::string& label, int* value, int low, int high);

    /**
     * A filled track spanning the width, with a line of text over it.
     * @param fraction how full, clamped to 0..1
     **/
    void progressBar(float fraction, const std::string& overlay);

    /**
     * A rule across the width, and the gap around it.
     **/
    void separator();

    /**
     * How wide the next widget should be, instead of the rest of the row.
     *
     * Only the next widget uses it, so it is set again before each one. Two scrubbers can
     * then share a row. It does not apply to a separator, which always spans the row.
     *
     * @param width in pixels, or nothing at all to go back to the rest of the row
     **/
    void nextItemWidth(float width);

    /**
     * Whether the cursor is over something this layer drew, or is dragging something it
     * drew.
     *
     * An app checks this before it acts on a click of its own, so that a press on a button
     * here does not also act on the scene behind it. input::Cursor's return values do the
     * same for the retained tree.
     *
     * Computed from what the previous frame found, the same way a widget's own hover is,
     * because an app calls this before it draws the current frame.
     **/
    bool capturing() const noexcept;

    /**
     * Claim a rectangle of the screen for the immediate ui, though it draws nothing there.
     *
     * For something another renderer draws, such as a modal backdrop over the whole viewport,
     * that should still stop a click reaching the scene below. The rectangle counts toward
     * capturing() exactly as a window does. It takes the cursor from everything drawn before
     * it, and a widget drawn after it inside it takes the cursor back. The result is one frame
     * behind.
     *
     * @param min the top left corner, in the canvas's pixels
     * @param max the bottom right corner
     **/
    void block(const glm::vec2& min, const glm::vec2& max);

    /**
     * A single line of text the user edits, drawn the width of the row.
     *
     * A press on it gives it the keyboard and puts the caret where the press landed. A
     * return, a tab, an escape or a press anywhere else gives the keyboard up, keeping what
     * was typed. A field that is not drawn for a frame gives it up too, and what was typed
     * since it was last drawn is lost.
     *
     * The label is the field's id, and is drawn dimmed in it while it is empty and does not
     * have the keyboard.
     *
     * @param value read and written. A change the caller makes while the field holds the
     *        keyboard replaces what is being edited
     * @param limit how many bytes the value may hold, or zero for no limit. Typing past it
     *        is refused
     * @return whether the value changed this frame
     **/
    bool textField(const std::string& label, std::string* value, std::size_t limit = 0);

    /**
     * A box that is ticked or not, with its label beside it.
     *
     * @param value read and written; flipped by a click
     * @return whether it was clicked this frame
     **/
    bool checkbox(const std::string& label, bool* value);

    /**
     * A number between two ends, set by pressing on the track or dragging along it.
     *
     * @param value read and written; left alone when nothing is held on it
     * @return whether the value changed this frame
     **/
    bool sliderFloat(const std::string& label, float* value, float low, float high);

    /**
     * The chosen one of several items, which a click opens as a list under it.
     *
     * The list is drawn at end(), over everything else, and is not clipped to the window
     * the combo is in. A click on an item takes effect on the next frame's call, as a click
     * on a tab does at endTabBar(). A press anywhere outside the list closes it, and so does
     * a frame that does not draw the combo.
     *
     * @param selected read and written - the index of the chosen item
     * @return whether the choice changed this frame
     **/
    bool combo(const std::string& label, int* selected, const std::vector<std::string>& items);

    /**
     * Whether the cursor is on the item just drawn, text and progress bars included, and
     * nothing drawn over it took the cursor. For a tooltip.
     *
     * Answers by the same one-frame-late rule every hover follows. A disabled widget
     * answers as text does, so a tooltip can say why it is disabled.
     **/
    bool itemHovered() const noexcept;

    /**
     * A key went down. A text field that holds the keyboard edits its line with it.
     *
     * Keys arrive between frames, as the app's event loop sees them, so the edit lands on
     * the line the layer keeps and reaches the caller's string when the field is next drawn.
     *
     * @param key the name api/input gives it
     * @param shifted whether a shift key is held, which selects as the caret moves
     * @param controlled whether a control key is held, for cut, copy, paste and select all
     * @return whether a field took it, in which case it does not reach the app's bindings.
     *      A field takes every key that composes text, so a letter typed into it does not
     *      also fire a command
     **/
    bool press(std::string_view key, bool shifted = false, bool controlled = false);

    /**
     * Characters the platform composed, put in at the caret of the field that holds the
     * keyboard.
     *
     * @return whether a field took them. Characters past the field's limit are taken and
     *      dropped
     **/
    bool typed(std::string_view utf8);

    /**
     * Whether a text field holds the keyboard. While it does, the platform's text input
     * should be on, and keys go to press() and typed() before they reach any binding.
     **/
    bool typing() const noexcept;

    /**
     * Be told when typing() changes, with what it changed to. shell::Keyboard listens here
     * to turn the platform's text input on and off.
     **/
    void onTyping(const std::function<void(bool)>& changed);

    /**
     * The clipboard a text field's cut, copy and paste read and write. Without one, a copy
     * and a paste do nothing and a cut leaves the text in place.
     **/
    void clipboard(const input::Clipboard& clipboard);

    /**
     * Put the next widget beside the last one rather than under it.
     **/
    void sameLine();

    /**
     * Draw everything until endDisabled() dimmed and unresponsive. The scopes nest, so a
     * disabled section inside a disabled section stays disabled after the inner one ends.
     **/
    void beginDisabled();
    void endDisabled();

    /**
     * Tell two widgets with the same label apart, by pushing something they do not share
     * onto the id stack.
     **/
    void pushId(const std::string& id);
    void pushId(int id);
    void popId();

    /**
     * Open a strip of tabs. The strip takes a row of its own, so what a selected tab holds
     * is drawn under the whole strip rather than beside the next tab.
     *
     * @return true, so that the call reads like the ones it wraps
     **/
    bool tabBar(const std::string& id);

    /**
     * One tab of the open strip.
     *
     * A click on a tab takes effect at endTabBar(), so the frame that switches tabs still
     * reports the tab that was selected when it began. Without that, both the old tab
     * and the new one would draw what they hold on the frame between them.
     *
     * @return whether this is the selected tab, and so whether to draw what it holds
     **/
    bool tab(const std::string& label);
    void endTabBar();

    /**
     * Open a table of a fixed number of columns. Name each with column() before the first
     * headerRow() or nextRow().
     *
     * A table given a height scrolls its own rows: it clips to that height, keeps its own
     * scroll offset across frames and draws its own bar down its right. Its header row is
     * drawn once above the region rather than in it. The wheel turns the innermost scrolling
     * region under the cursor, so a table inside a window takes the wheel from the window.
     * A table given no height is as tall as its rows and scrolls with whatever holds it.
     *
     * The room a bar would take is reserved whether or not there is anything to scroll,
     * so the columns of a table that gains a row do not re-flow.
     *
     * The shape is how many columns the table has, and how tall it is, or zero to be as tall
     * as its rows.
     **/
    struct TableShape {
        unsigned int columns = 1;
        float height = 0.0F;
    };

    bool table(const std::string& id, const TableShape& shape);

    /**
     * Name a column and say how wide it is. A width of zero shares out what the other
     * columns left.
     **/
    void column(const std::string& label, float width);

    /**
     * Draw the header row - the column names on a band of their own.
     *
     * In a table that scrolls, the band is drawn above the scrolled region, not in it. The
     * rows pass under the band.
     **/
    void headerRow();

    /**
     * Start a row, at the first column.
     **/
    void nextRow();

    /**
     * Move to the next column of the row being written.
     **/
    void nextColumn();
    void endTable();

 protected:
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
     * What a widget is holding, marked as still wanted so that end() does not age it out.
     **/
    Retained& retain(Id id);

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
     * Draw the open combo's list over everything, and take a click on one of its items.
     **/
    void popup();

    /**
     * Give the keyboard to a text field, or take it from whichever has it, and tell
     * whoever listens to onTyping().
     **/
    void focus(Id field);

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
     * Two things have to agree for a widget to be hovered: the cursor is in its box now,
     * and nothing drawn over it took the cursor last frame. The first stops a widget
     * reacting to a release that landed somewhere else; the second lets a window drawn
     * later take the cursor from one under it.
     *
     * A disabled widget is offered nothing, which stops it lighting up as well as
     * answering.
     **/
    Reaction interact(Id id, const glm::vec2& min, const glm::vec2& max);

    /**
     * Draw a line of text with its box already worked out, vertically centred in it.
     **/
    void label(const std::string& line, const glm::vec2& min, const glm::vec2& size, const glm::vec4& colour) const;

    /**
     * Whether a scrolled region puts up its scrollbar: a window decides at its start, from
     * what it drew last frame, and a table whenever there is more than it shows.
     **/
    enum class Bar : std::uint8_t { Never, Always, WhenNeeded };

    /**
     * A scrolled region. id holds its scroll and content, and scroll is the id its
     * scrollbar is hit tested under. contentTop is where its content would start if it were
     * not scrolled. bodyMax is the bottom right corner of its body, which the scrollbar runs
     * down to.
     **/
    struct Scrolled {
        Id id = 0;
        Id scroll = 0;
        float contentTop = 0.0F;
        glm::vec2 bodyMax{0.0F};
    };

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
    void scrollbar(Id id, const glm::vec2& min, const glm::vec2& max, float view, float span,
        float* scroll);

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

    /**
     * @return the colour to draw text in, given what the caller asked for and whether
     *      anything has disabled it
     **/
    glm::vec4 ink(const glm::vec4& colour) const;

    /**
     * @return what to fill a widget's box with
     * @param lit whether it is held down or chosen, which read the same way
     * @param hovered whether the cursor is on it
     **/
    glm::vec4 face(bool lit, bool hovered) const;

 private:
    paint::Measure measure_;
    paint::Write write_;
    Dressing dressing_;
    // what the current theme wrote over dressing_
    style::Overlay<Dressing> overlay_;
    boost::shared_ptr<style::Theme> theme_;

    v3d::render::realtime::canvas::Canvas* canvas_;
    Input input_;
    glm::vec2 previousCursor_;
    glm::vec2 drag_;

    Id hovered_;   /**< what the cursor was on last frame, which this frame uses **/
    Id hovering_;  /**< what it is on this frame, which the next one will use **/
    Id active_;    /**< what a press went down on **/
    glm::vec2 pressAt_;  /**< and where it went down, which a travel is measured from **/
    float nextWidth_;  /**< what the next widget was told to be, or nothing **/

    std::vector<Id> ids_;
    std::map<Id, Retained> state_;
    std::uint64_t frame_;

    Row row_;
    Window window_;
    TabStrip tabs_;
    Table table_;
    unsigned int disabled_;
    Id wheeled_;  /**< the topmost window the cursor is over, which the wheel turns **/

    Id typing_;            /**< the text field that holds the keyboard, or nothing **/
    bool letGo_;           /**< a return, tab or escape asked the field to give it up **/
    bool typingDrawn_;     /**< whether that field has been drawn this frame **/
    std::string written_;  /**< what the field last wrote to the caller's string **/
    input::LineEdit line_;
    input::Clipboard clipboard_;
    std::function<void(bool)> onTyping_;

    glm::vec2 itemMin_;  /**< the box of the item placed last, which itemHovered() asks about **/
    glm::vec2 itemMax_;
    Id itemOwner_;       /**< what has to have held the cursor for that item to be hovered **/

    Popup popup_;
};

};  // namespace v3d::ui
