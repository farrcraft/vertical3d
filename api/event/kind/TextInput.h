/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>

#include <string>

namespace v3d::event::kind {

/**
 * Characters the platform composed, as utf-8.
 *
 * Separate from KeyDown because one key does not map to one character. The same key is "a"
 * and "A" under shift. A dead key and the one after it are one character between them. An
 * input method may compose several keys into one. A text box needs the characters the
 * platform composed, so this carries them, and KeyDown carries which key moved.
 **/
class TextInput final : public Event {
 public:
    /**
     * @param text one or more characters, utf-8 encoded
     **/
    TextInput(const std::string& text, const boost::shared_ptr<Context>& context);

    /**
     * @return what was composed, which is never empty
     **/
    std::string_view text() const noexcept;

 private:
    std::string text_;
};

};  // namespace v3d::event::kind
