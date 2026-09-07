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
 * It is a check box because it is the same state under a different mark: the click sends
 * a command and marks nothing, and whatever answers the command checks this one and
 * clears the rest of its group. Nothing here clears them, for the reason nothing here
 * checks anything - ADR-0019.
 *
 * The mark and the outline are the "radio" style class the component names, per ADR-0020.
 **/
class RadioButton : public CheckBox {
 public:
    RadioButton();
    ~RadioButton() = default;

    /**
     * Set which set of choices this is one of. The name means nothing to the library and
     * everything to whatever answers the command.
     **/
    void group(const std::string& name);
    std::string_view group() const;

 private:
    std::string group_;
};

};  // namespace v3d::ui::component
