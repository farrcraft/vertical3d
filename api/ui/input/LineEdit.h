/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/paint/Text.h>

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace v3d::ui::input {

/**
 * The platform's clipboard, as a pair of callbacks. This library cannot reach the clipboard
 * without depending on SDL, so the app supplies it, as it supplies text measuring.
 *
 * An editor given neither callback still handles every other edit. A cut with no write
 * callback leaves the text in place, so the text is never lost. A paste with no read callback
 * inserts nothing.
 **/
struct Clipboard final {
    std::function<std::string()> read;
    std::function<void(std::string_view)> write;
};

/**
 * One line of utf-8 text being edited, with a caret and a selection in it.
 *
 * The caret is a byte offset into the text, and every operation moves it to a character
 * boundary, so a multi-byte character is inserted, stepped over and erased whole.
 *
 * A selection is an anchor plus the caret, two byte offsets. Nothing is selected exactly when
 * the two are equal. Every operation that moves the caret says whether the anchor moves with
 * it. Cut, copy and paste are then a run of bytes and an insertion.
 *
 * The retained text box and the immediate text field both edit through one of these.
 **/
class LineEdit final {
 public:
    LineEdit() noexcept;

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
     * Where the selection starts, as a byte offset. It is wherever the caret is when nothing
     * is selected, so there is no third piece of state to keep in step.
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
     * @return whether anything is selected, which is whether the anchor and the caret are in
     *      different places
     **/
    bool selected() const noexcept;

    /**
     * @return the selected run, empty when nothing is selected. It is a view into the
     *      editor's own text, not a copy of it
     **/
    std::string_view selection() const noexcept;

    /**
     * Take the selected run out, leaving the caret where the run started.
     *
     * @return whether anything was selected
     **/
    bool removeSelection();

    /**
     * How many bytes the text may hold, or zero for no limit. An insertion that would go past
     * it is refused whole rather than truncated, so a paste either lands or does not.
     **/
    void limit(std::size_t bytes) noexcept;
    std::size_t limit() const noexcept;

    /**
     * Put characters in over the selection, or at the caret when nothing is selected, and
     * leave the caret after them.
     *
     * @param value utf-8, as the platform composed it or as a clipboard held it
     * @return whether it went in. Only an empty value or the limit refuses it. The limit is
     *      measured against what the text would become, so a paste may be as long as the run
     *      it replaces plus whatever room was left
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
     * Which character boundary a distance into the drawn line falls on, so that a press can
     * put the caret where it landed.
     *
     * Makes one measure call per character boundary. Per-character widths do not add up to
     * what a run measures, so each boundary is measured as a run.
     *
     * @param into how far past the start of the line the point is, in pixels
     * @param measure how wide a run of the text is when it is drawn
     * @return the byte offset of the boundary nearest the point
     **/
    std::size_t at(float into, const paint::Measure& measure) const;

 private:
    /**
     * The character boundary at or before an offset, so a caret never lands inside a
     * multi-byte character however it was asked to move.
     **/
    std::size_t boundary(std::size_t offset) const noexcept;

    /**
     * The character boundary after an offset, which is the end of the character that starts
     * there.
     **/
    std::size_t forward(std::size_t offset) const noexcept;

    /**
     * @return where the selected run starts and where it ends: the lower of the anchor and
     *      the caret first, then the higher
     **/
    std::pair<std::size_t, std::size_t> run() const noexcept;

    std::string text_;
    std::size_t caret_;
    std::size_t anchor_;
    std::size_t limit_;
};

/**
 * Carry out the edit a key names on a line: a caret move, a deletion, or a cut, copy, paste
 * or select all chord.
 *
 * A key that will arrive again as a composed character is taken without an edit, so that a
 * letter typed into a line does not also reach the app's bindings. A return is not handled
 * here, because what it does depends on what holds the line.
 *
 * @param key the name api/input gives it - "backspace", "arrow_left", "a"
 * @param shifted whether a shift key is held. Shift and a caret key selects the run moved over
 * @param controlled whether a control key is held. Ctrl with c, x, v and a is copy, cut, paste
 *        and select all. Every other chord goes on to the app
 * @return whether the line took the key
 **/
bool edit(LineEdit* line, std::string_view key, bool shifted, bool controlled, const Clipboard& clipboard);

};  // namespace v3d::ui::input
