/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include "Setting.h"

namespace v3d::ui::paint {

/**
 * What a centred menu sets apart from the text of its items: its title, and the label of the
 * chosen item. A setting left unset is set as the body text is.
 **/
struct Emphasis final {
    Setting title;
    Setting chosen;
};

};  // namespace v3d::ui::paint
