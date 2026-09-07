/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "Style.h"
#include "style/Theme.h"

#include "../render/realtime/Canvas.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui {

/**
 * A ui written as calls rather than as a tree, per ADR-0035.
 *
 * A panel is a sequence between begin() and end(): each widget is placed where the layout
 * pen has got to, drawn onto the same quad canvas the retained components draw onto, hit
 * tested against the box it was just drawn in, and answers on the spot. There is nothing
 * to keep in step with the state it shows, because it is a function of that state - which
 * is what makes it the right shape for a tool and the wrong one for a hud.
 *
 * Text is the caller's to draw, the same way ComponentRenderer takes it: this class knows
 * how wide a string is and where it goes, and v3d::font turns it into glyphs.
 *
 * Which widget the cursor is on is decided as a frame is drawn and used by the next one,
 * so a widget drawn later takes the cursor from one under it. A widget that moved is
 * therefore hovered a frame late.
 *
 * A window cuts what it holds off at its own edges and scrolls it, which is the canvas's
 * clip of ADR-0037. How tall the content is is measured as it is drawn, so a window
 * decides whether it needs a scrollbar from what the frame before it held.
 **/
class Immediate {
 public:
    /**
     * How wide a string will be when it is drawn, in pixels.
     **/
    typedef std::function<float(const std::string&)> Measure;

    /**
     * Draw a string with its pen on the baseline at the given position.
     **/
    typedef std::function<void(const std::string&, const glm::vec2&, const glm::vec4&)> Write;

    /**
     * What a widget is known by. A label hashed with whatever pushId() has put on the
     * stack, so two widgets with the same label in the same scope are one widget as far
     * as the cursor is concerned.
     **/
    typedef std::uint64_t Id;

    /**
     * What the cursor did since the last frame. An app fills this from its input engine;
     * pressed and released are the edges of down, and are true for the one frame each.
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
     * The colours and metrics the layer draws with. What a theme's "ui" style does not
     * name keeps the value here, so a theme carrying nothing changes nothing.
     *
     * Not a Style, which is the bag of properties a theme holds. This is what one resolves
     * to.
     **/
    struct Dressing final {
        Dressing() noexcept;

        float lineHeight;      /**< the height of one row of text **/
        float padding;         /**< the gap between a widget's edge and the text in it **/
        float spacing;         /**< the gap between one widget and the next **/
        float barHeight;       /**< how tall a title bar or a tab is **/
        float borderWidth;     /**< how thick a window's outline is **/
        float radius;          /**< how far a window's corners are rounded **/
        float indent;          /**< how far a bullet pushes its text in **/
        float scrollbarWidth;  /**< how wide the bar down a window that scrolls is **/
        glm::vec4 panel;       /**< a window's background **/
        glm::vec4 border;      /**< its outline **/
        glm::vec4 titleBar;    /**< the band across the top of one **/
        glm::vec4 text;        /**< ordinary text **/
        glm::vec4 activeText;  /**< text on something the cursor is on **/
        glm::vec4 dimText;     /**< text that is disabled, or said to be unimportant **/
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
    Immediate(const Measure& measure, const Write& write);

    /**
     * @return the colours and metrics, to be changed in place
     **/
    Dressing& dressing() noexcept;

    /**
     * Read a theme's "ui" style into dressing(), per ADR-0020. The same style the retained
     * components are dressed by, so the two ways of writing a ui look like one.
     **/
    void theme(const boost::shared_ptr<style::Theme>& theme);

    /**
     * Start a frame. Everything drawn until end() goes onto this canvas.
     **/
    void begin(v3d::render::realtime::Canvas* canvas, const Input& input);

    /**
     * Finish a frame, which is what settles who has the cursor for the next one, and ages
     * out what nothing has drawn for retention frames.
     **/
    void end();

    /**
     * How many widgets the layer is holding something for.
     *
     * A window's scroll and fold and a tab strip's selection are all it keeps, so this is
     * a handful in a running app and is here to say so - a caller building ids out of
     * changing text can watch it rather than discover the cost later.
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
     * Open a window at a place the caller decides - this layer does not drag one.
     *
     * What goes in it is cut off at the window's edges and scrolls when there is more of
     * it than fits, per ADR-0037. How much there is is what last frame's content came to,
     * so the bar appears on the frame after the one that overflowed and a window whose
     * content changes every frame sizes its thumb a frame behind.
     *
     * Pair every call with endWindow() whatever it answered: a collapsed window returns
     * false and still has to be closed.
     *
     * @param alpha what the window's background is drawn at, 1 for opaque
     * @return whether what goes in it should be drawn, which a collapsed window says no to
     **/
    bool window(const std::string& title, const glm::vec2& position, const glm::vec2& size, float alpha);
    void endWindow();

    /**
     * One line of text at the pen.
     **/
    void text(const std::string& line);

    /**
     * A line of text in the dim colour, for something said rather than shown.
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
     * answers for the tab that was selected when it began. Without that, both the old tab
     * and the new one would draw what they hold on the frame between them.
     *
     * @return whether this is the selected tab, and so whether to draw what it holds
     **/
    bool tab(const std::string& label);
    void endTabBar();

    /**
     * Open a table of a fixed number of columns. Name each with column() before the first
     * headerRow() or nextRow().
     **/
    bool table(const std::string& id, unsigned int columns);

    /**
     * Name a column and say how wide it is. A width of zero shares out what the other
     * columns left.
     **/
    void column(const std::string& label, float width);

    /**
     * Draw the header row - the column names on a band of their own.
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

        std::uint64_t frame;  /**< the last frame that asked for it, which is what ages it out **/
        unsigned int tab;  /**< which tab of a strip is selected **/
        float scroll;      /**< how far the window's content is scrolled up, in pixels **/
        float content;     /**< how tall what it held came to last frame **/
        bool collapsed;    /**< whether a window is folded to its title bar **/
    };


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
     * Offer a box to the cursor.
     *
     * Two things have to agree for a widget to be hovered: the cursor is in its box now,
     * and nothing drawn over it took the cursor last frame. The first is what stops a
     * widget answering a release that landed somewhere else; the second is what lets a
     * window drawn later take the cursor from one under it.
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
     * Draw the bar down the right of a window that has more content than it shows, and
     * scroll it where the cursor drags the thumb to.
     *
     * @param view how much of the content the window shows, in pixels
     * @param span how much of it it does not, which is the furthest it can be scrolled
     * @param scroll read and written - where the window is scrolled to
     **/
    void scrollbar(float view, float span, float* scroll);

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
    Measure measure_;
    Write write_;
    Dressing dressing_;
    boost::shared_ptr<style::Theme> theme_;

    v3d::render::realtime::Canvas* canvas_;
    Input input_;
    glm::vec2 previousCursor_;
    glm::vec2 drag_;

    Id hovered_;   /**< what the cursor was on last frame, which is what answers this one **/
    Id hovering_;  /**< what it is on this frame, which the next one will use **/
    Id active_;    /**< what a press went down on **/

    std::vector<Id> ids_;
    std::map<Id, Retained> state_;
    std::uint64_t frame_;

    float margin_;      /**< where a new row starts **/
    float right_;       /**< where the room a widget may take ends **/
    float penY_;        /**< where the next row starts **/
    float rowTop_;      /**< the top of the row being written **/
    float rowHeight_;   /**< the tallest thing on it so far **/
    float lastRight_;   /**< just past the last widget, which sameLine() starts from **/
    bool sameLine_;

    unsigned int disabled_;

    /**< the window being written, and what it interrupted **/
    bool inWindow_;
    float windowMargin_;
    float windowRight_;
    Id window_;          /**< whose scroll and content the one being written are **/
    Id windowScroll_;    /**< the id its scrollbar answers the cursor as **/
    glm::vec2 bodyMin_;  /**< the part of it below the title bar, which is what is cut to **/
    glm::vec2 bodyMax_;
    float contentTop_;   /**< where its content would start if it were not scrolled **/
    bool windowScrolls_;
    bool windowClipped_;
    Id wheeled_;         /**< the topmost window the cursor is over, which the wheel turns **/

    /**< the tab strip being written **/
    Id tabBar_;
    bool inTabBar_;
    float tabPen_;
    float tabTop_;
    unsigned int tabIndex_;
    unsigned int tabWanted_;
    bool tabChanged_;
    bool tabTaken_;

    /**< the table being written **/
    bool inTable_;
    std::vector<std::string> headers_;
    std::vector<float> widths_;
    float tableLeft_;
    unsigned int columnIndex_;
};

};  // namespace v3d::ui
