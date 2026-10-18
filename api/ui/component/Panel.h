/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Component.h>

namespace v3d::ui::component {

/**
 * A filled rectangle with a border, used for backgrounds and backdrops.
 *
 * A panel holds no colours of its own. Its fill, its border colour and its corner radius
 * come from the "panel" style class the component names. A panel with no style draws in
 * the renderer's defaults.
 **/
class Panel : public Component {
 public:
    Panel();
    ~Panel() = default;
};

};  // namespace v3d::ui::component
