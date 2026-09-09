/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Component.h>

namespace v3d::ui::component {

/**
 * A box - a filled rectangle with a border, and the thing every plate, track and backdrop
 * is made of.
 *
 * It carries no colours of its own: what it is filled with, what its border is drawn in
 * and how far its corners are rounded are the "panel" style class the component names,
 * per ADR-0020. A panel with no style draws in the renderer's defaults.
 **/
class Panel : public Component {
 public:
    Panel();
    ~Panel() = default;
};

};  // namespace v3d::ui::component
