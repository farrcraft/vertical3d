/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>
#include <api/ui/Component.h>

#include <string>

namespace v3d::ui::component {

/**
 * A box with a mark in it and a label beside it.
 *
 * It does not own the state it shows, for the reason a toggle button does not: clicking
 * one sends its command and marks nothing, and whatever answers the command sets
 * checked(). See ADR-0019. A check box that is never told is never checked, which is a
 * missing handler rather than a missing mark.
 *
 * The box, the mark and the outline are the "checkbox" style class the component names,
 * per ADR-0020.
 **/
class CheckBox : public Component {
 public:
    CheckBox();
    ~CheckBox() = default;

    /**
     * Set the text drawn beside the box.
     **/
    void label(const std::string& str);
    std::string_view label() const;

    /**
     * Set whether the mark is drawn.
     **/
    void checked(bool on);
    bool checked() const;

    /**
     * Set the event clicking it sends.
     **/
    void event(const v3d::event::Event& destination);
    v3d::event::Event event() const;

 protected:
    /**
     * For a radio button, which is the same state and the same command under a round
     * mark.
     **/
    explicit CheckBox(Type type);

 private:
    std::string label_;
    v3d::event::Event event_;
    bool checked_;
};

};  // namespace v3d::ui::component
