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
 * Distinct from a KeyDown because a key is not a character: the same key is "a" and "A"
 * under shift, a dead key and the one after it are one character between them, and an
 * input method may compose several keys into one. What a text box wants is the answer the
 * platform arrived at, so this carries it and KeyDown goes on carrying which key moved.
 * ADR-0040.
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
