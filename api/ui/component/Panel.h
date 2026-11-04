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
 *
 * A panel can instead be the recess a set of tabs sits in, drawn behind a row of buttons with
 * a tab's look. It is filled with the dressing's tab colour, rounded, with no outline.
 **/
class Panel : public Component {
 public:
    Panel();
    ~Panel() = default;

    /**
     * @param on whether the panel is the recess a set of tabs sits in
     **/
    void recess(bool on) noexcept;

    /**
     * @return whether the panel is the recess a set of tabs sits in
     **/
    bool recess() const noexcept;

 private:
    bool recess_;
};

};  // namespace v3d::ui::component
