/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/geometry/Bound2D.h>
#include <api/ui/input/LineEdit.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/Text.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::render::realtime::canvas {
class Canvas;
};  // namespace v3d::render::realtime::canvas

namespace v3d::ui {

namespace style {
class Theme;
};  // namespace style

namespace immediate {
class Context;
};  // namespace immediate

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
     * @param measure how wide a string is when the app draws it
     * @param write how the app draws a string
     **/
    Immediate(const paint::Measure& measure, const paint::Write& write);
    ~Immediate();

    // what the layer holds is its own, so it moves and is not copied
    Immediate(Immediate&&) noexcept;
    Immediate& operator=(Immediate&&) noexcept;
    Immediate(const Immediate&) = delete;
    Immediate& operator=(const Immediate&) = delete;

    /**
     * Replace how strings are measured and drawn, as when the text is rescaled. Everything
     * the layer remembers between frames - where windows were dragged, what is folded, how
     * far a list is scrolled - is kept.
     **/
    void text(const paint::Measure& measure, const paint::Write& write);

    /**
     * @return the colours and metrics, to be changed in place. They start from
     *      paint::Dressing::tools()
     **/
    paint::Dressing& dressing() noexcept;

    /**
     * Read a theme's "tools" style into dressing(). The theme is not kept.
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
     * A line of text in the disabled-text colour, for secondary text.
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
     * The label is the field's id, and is drawn in it in the placeholder colour while it is
     * empty and does not have the keyboard.
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

 private:
    std::unique_ptr<immediate::Context> context_;
};

};  // namespace v3d::ui
