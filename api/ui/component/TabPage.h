/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../Component.h"

namespace v3d::ui::component {

/**
 * One page of a tab bar - a label on the strip, and whatever the page holds under it.
 *
 * A page draws nothing of its own: it is a box that holds components, the way a flow box
 * is, and only the chosen page of a bar is walked at all. What it holds is laid out
 * against the room the strip left, per ADR-0034.
 **/
class TabPage : public Component {
 public:
    TabPage();
    ~TabPage() = default;

    /**
     * Set the text on the page's tab.
     **/
    void label(const std::string& str);
    std::string_view label() const;

 private:
    std::string label_;
};

};  // namespace v3d::ui::component
