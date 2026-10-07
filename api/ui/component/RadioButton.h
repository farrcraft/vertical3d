/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "CheckBox.h"

namespace v3d::ui::component {

/**
 * One of a set of choices - a check box with a round mark and the name of the set it
 * belongs to.
 *
 * It derives from CheckBox because it holds the same state under a different mark. A
 * click sends a command and marks nothing. Whatever handles the command checks this one
 * and clears the rest of its group; nothing here clears them.
 *
 * The mark and the outline are the "radio" style class the component names.
 **/
class RadioButton : public CheckBox {
 public:
    RadioButton();
    ~RadioButton() = default;

    /**
     * Set which set of choices this is one of. The library does not interpret the name;
     * whatever handles the command does.
     **/
    void group(const std::string& name);
    std::string_view group() const;

 private:
    std::string group_;
};

};  // namespace v3d::ui::component
