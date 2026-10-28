/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>
#include <api/ui/Component.h>
#include <api/ui/input/LineEdit.h>
#include <api/ui/paint/Text.h>

#include <cstddef>
#include <string>
#include <string_view>

namespace v3d::ui::component {

/**
 * One line of text the user can edit, with a caret in it.
 *
 * A text box holds the text it shows, the way a SelectList holds its rows, because a
 * half-typed word has nowhere else to be kept. Whatever handles its command reads text().
 *
 * The editing is input::LineEdit's, and every editing call here is that class's call on the
 * box's line. The key routing is in input::Keys, which chooses the operation for a key.
 *
 * The box never reaches the clipboard. input::Keys receives clipboard access as callbacks,
 * so api/ui names no SDL type. input::Cursor measures text through the app's Measure
 * callback, so a press places the caret through at() and a drag selects.
 *
 * The background, the text and the caret are the "textbox" style class.
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
     * @return the selected run, empty when nothing is selected. A copy writes this to the
     *      clipboard. It is a view into the box's own text, not a copy of it
     **/
    std::string_view selection() const noexcept;

    /**
     * Take the selected run out, leaving the caret where the run started.
     *
     * A cut calls this after writing the run to the clipboard. A backspace, a delete and an
     * insertion each call it first when there is a selection.
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
     * What is drawn in place of the text while the box is empty and does not have the
     * focus. It is never edited and never returned by text().
     **/
    void placeholder(const std::string& value);
    std::string_view placeholder() const noexcept;

    /**
     * Put characters in over the selection, or at the caret when nothing is selected, and
     * leave the caret after them.
     *
     * @param value utf-8, as the platform composed it or as a clipboard held it
     * @return whether it went in. Only an empty value or the limit refuses it. The limit is
     *      measured against what the text would become, so a paste may be as long as the
     *      run it replaces plus whatever room was left
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
     * With a selection and extend false, left() moves the caret to the start of the
     * selection and right() to its end, not a character past it, as in other text editors.
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
     * The line the box edits, for input::edit() to carry out a key on.
     **/
    input::LineEdit* line() noexcept;

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
    input::LineEdit line_;
    std::string placeholder_;
    v3d::event::Event event_;
    float pen_;
};

};  // namespace v3d::ui::component
