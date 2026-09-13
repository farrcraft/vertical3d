/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>
#include <api/ui/Component.h>
#include <api/ui/paint/Text.h>

#include <cstddef>
#include <string>
#include <utility>

namespace v3d::ui::component {

/**
 * One line of text the user can edit, with a caret in it.
 *
 * A text box owns what it shows, the way a SelectList owns its rows: the text is a place
 * in the component rather than a fact about the app, and there is nowhere else for a
 * half-typed word to live. Whatever answers its command reads text() when it arrives.
 *
 * The editing is here and the routing is ui::Keys' - a key names an operation and this
 * carries it out, the way a cursor names a row and a list chooses it. ADR-0040.
 *
 * The caret is a byte offset into utf-8, and every operation moves it to a character
 * boundary, so a multi-byte character is inserted, stepped over and erased whole.
 *
 * A selection is an anchor the caret has moved away from rather than a flag and a range -
 * ADR-0057. Nothing is selected exactly when the two are in the same place, so every
 * operation that moves the caret decides one thing: whether the anchor comes with it. A
 * cut, a copy and a paste are then a run of bytes and an insertion, which the box had
 * already.
 *
 * The plate, the text and the caret are the "textbox" style class, per ADR-0020.
 **/
class TextBox : public Component {
 public:
    TextBox();
    ~TextBox() = default;

    /**
     * Replace the text, leaving the caret at the end of it and nothing selected.
     **/
    void text(const std::string& value);
    std::string_view text() const noexcept;

    /**
     * Where the next character goes, as a byte offset into text().
     *
     * Clamped to the text and moved to a character boundary, so an offset inside a
     * multi-byte character is not expressible.
     *
     * @param extend whether the anchor stays where it is, which is what makes the move
     *        select the run travelled rather than leave it behind
     **/
    void caret(std::size_t offset, bool extend = false);
    std::size_t caret() const noexcept;

    /**
     * Where the selection starts, as a byte offset. It is wherever the caret is when
     * nothing is selected, so there is no third piece of state to keep in step.
     **/
    std::size_t anchor() const noexcept;

    /**
     * Select a run, leaving the caret at `to` - the end a shift and an arrow moves.
     **/
    void select(std::size_t from, std::size_t to);

    /**
     * Select the whole of the text, with the caret at the end of it.
     **/
    void selectAll();

    /**
     * Drop the selection, leaving the caret where it is.
     **/
    void deselect() noexcept;

    /**
     * @return whether anything is selected, which is whether the anchor and the caret are
     *      in different places
     **/
    bool selected() const noexcept;

    /**
     * @return the selected run, empty when nothing is selected. What a copy hands a
     *      clipboard, and a view into the box's own text rather than a copy of it
     **/
    std::string_view selection() const noexcept;

    /**
     * Take the selected run out, leaving the caret where the run started.
     *
     * What a cut does once the run has been handed over, and what a backspace, a delete and
     * an insertion each do to a selection before doing anything of their own.
     *
     * @return whether anything was selected
     **/
    bool removeSelection();

    /**
     * How many bytes the box will hold, or zero for no limit. An insertion that would go
     * past it is refused whole rather than truncated, so a paste either lands or does not.
     **/
    void limit(std::size_t bytes) noexcept;
    std::size_t limit() const noexcept;

    /**
     * What is drawn when the box is empty, in place of the text. It is never edited and
     * never returned by text().
     **/
    void placeholder(const std::string& value);
    std::string_view placeholder() const noexcept;

    /**
     * Put characters in over the selection, or at the caret when nothing is selected, and
     * leave the caret after them.
     *
     * @param value utf-8, as the platform composed it or as a clipboard held it
     * @return whether it went in, which the limit is what refuses. The limit is measured
     *      against what the text would become, so a paste may be as long as the run it
     *      replaces plus whatever room was left
     **/
    bool insert(std::string_view value);

    /**
     * Take out the character before the caret and step back over it, or the selected run
     * when there is one.
     * @return whether there was anything to take out
     **/
    bool backspace();

    /**
     * Take out the character at the caret and leave the caret where it is, or the selected
     * run when there is one.
     * @return whether there was anything to take out
     **/
    bool erase();

    /**
     * Move the caret one character, or to an end of the text.
     *
     * A selection has two ends, so an arrow with nothing held lands on the near or the far
     * one rather than a character past it - which is what a first arrow out of a selection
     * means everywhere else.
     *
     * @param extend whether the anchor stays where it is, which selects the run travelled
     * @return whether anything moved
     **/
    bool left(bool extend = false);
    bool right(bool extend = false);
    bool home(bool extend = false);
    bool end(bool extend = false);

    /**
     * Which character boundary a point in the box falls on, so that a press can put the
     * caret where it landed.
     *
     * Measured against the pen the last draw left, per ADR-0019: a box that has not been
     * drawn answers the start of its text, the same way a component that has not been drawn
     * cannot be picked at all. The point is in canvas pixels and only its x is read,
     * because a box holds one line.
     *
     * One measure call per character boundary, which is what a click costs on a line short
     * enough to be held in one box. The alternative is adding up per character widths, and
     * those do not come to what a run measures.
     *
     * @param measure how wide a run of the text is when it is drawn
     * @return the byte offset of the boundary nearest the point
     **/
    std::size_t at(const glm::vec2& point, const paint::Measure& measure) const;

    /**
     * Where the text was last drawn from, in canvas pixels: the inside edge of the box, less
     * however far the line slid to keep the caret in view.
     *
     * Left here by whatever drew it, the way a list's row height is. The slide is a fact
     * about the draw rather than about the box, and at() cannot find a character without it.
     **/
    void pen(float x) noexcept;
    float pen() const noexcept;

    /**
     * Set the event a return in the box sends. What the text then means is the app's -
     * the box is told the user is done and sends its command, and whatever answers reads
     * text().
     **/
    void event(const v3d::event::Event& destination);
    v3d::event::Event event() const;

 private:
    /**
     * The character boundary at or before an offset, so a caret never lands inside a
     * multi-byte character however it was asked to move.
     **/
    std::size_t boundary(std::size_t offset) const noexcept;

    /**
     * The character boundary after an offset, which is the end of the character that
     * starts there.
     **/
    std::size_t forward(std::size_t offset) const noexcept;

    /**
     * @return where the selected run starts and where it ends, which is the anchor and the
     *      caret in that order
     **/
    std::pair<std::size_t, std::size_t> run() const noexcept;

    std::string text_;
    std::string placeholder_;
    v3d::event::Event event_;
    std::size_t caret_;
    std::size_t anchor_;
    std::size_t limit_;
    float pen_;
};

};  // namespace v3d::ui::component
