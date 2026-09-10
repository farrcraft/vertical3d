/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>
#include <api/ui/Component.h>

#include <cstddef>
#include <string>

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
 * The plate, the text and the caret are the "textbox" style class, per ADR-0020.
 **/
class TextBox : public Component {
 public:
    TextBox();
    ~TextBox() = default;

    /**
     * Replace the text, leaving the caret at the end of it.
     **/
    void text(const std::string& value);
    std::string_view text() const noexcept;

    /**
     * Where the next character goes, as a byte offset into text().
     *
     * Clamped to the text and moved to a character boundary, so an offset inside a
     * multi-byte character is not expressible.
     **/
    void caret(std::size_t offset);
    std::size_t caret() const noexcept;

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
     * Put characters in at the caret and leave the caret after them.
     *
     * @param value utf-8, as the platform composed it
     * @return whether it went in, which the limit is what refuses
     **/
    bool insert(std::string_view value);

    /**
     * Take out the character before the caret, and step back over it.
     * @return whether there was one
     **/
    bool backspace();

    /**
     * Take out the character at the caret, leaving the caret where it is.
     * @return whether there was one
     **/
    bool erase();

    /**
     * Move the caret one character, or to an end of the text.
     * @return whether it moved
     **/
    bool left();
    bool right();
    bool home();
    bool end();

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

    std::string text_;
    std::string placeholder_;
    v3d::event::Event event_;
    std::size_t caret_;
    std::size_t limit_;
};

};  // namespace v3d::ui::component
