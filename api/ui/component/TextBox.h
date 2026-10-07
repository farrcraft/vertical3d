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
 * A text box owns what it shows, the way a SelectList owns its rows. The text is a place
 * in the component rather than a fact about the app, and there is nowhere else for a
 * half-typed word to live. Whatever handles its command reads text() when it arrives.
 *
 * The editing is here and the key routing is in ui::Keys: a key names an operation and
 * this component carries it out.
 *
 * The caret is a byte offset into utf-8, and every operation moves it to a character
 * boundary, so a multi-byte character is inserted, stepped over and erased whole.
 *
 * A selection is an anchor plus the caret, two byte offsets. Nothing is selected exactly
 * when the two are equal. Every operation that moves the caret says whether the anchor
 * moves with it. Cut, copy and paste are then a run of bytes and an insertion.
 *
 * The box never reaches the clipboard. ui::Keys receives clipboard access as callbacks,
 * so api/ui names no SDL type. ui::Cursor measures text through the app's Measure
 * callback, so a press places the caret through at() and a drag selects.
 *
 * The plate, the text and the caret are the "textbox" style class.
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
     * @param extend whether the anchor stays where it is, so the move selects the run
     *        travelled
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
     * @return whether it went in. Only the limit refuses it, and the limit is measured
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
     * one rather than a character past it, as in other text editors.
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
     * Measured against the pen position the last draw stored, so a box that has not been
     * drawn returns the start of its text. The point is in canvas pixels and only its x is
     * read, because a box holds one line.
     *
     * Makes one measure call per character boundary. Per-character widths do not add up to
     * what a run measures, so each boundary is measured as a run.
     *
     * @param measure how wide a run of the text is when it is drawn
     * @return the byte offset of the boundary nearest the point
     **/
    std::size_t at(const glm::vec2& point, const paint::Measure& measure) const;

    /**
     * Where the text was last drawn from, in canvas pixels: the inside edge of the box, less
     * however far the line slid to keep the caret in view.
     *
     * Stored by whatever drew the box, like a list's row height. at() needs it to find a
     * character.
     **/
    void pen(float x) noexcept;
    float pen() const noexcept;

    /**
     * Set the event a return in the box sends. Whatever handles the command reads text()
     * and decides what it means.
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
