/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/ui/immediate/Id.h>

namespace v3d::ui::immediate {

/**
 * The tab strip being written.
 **/
struct TabStrip final {
    TabStrip() noexcept;

    Id id;
    bool open;
    float pen;              /**< how far along the strip the next tab goes **/
    float top;
    float share;            /**< how wide each tab is, or 0 for as wide as its label **/
    unsigned int index;     /**< which tab of the strip this one is **/
    unsigned int wanted;    /**< the tab a click asked for, taken at endTabBar() **/
    bool changed;
    bool taken;             /**< whether the selected tab was drawn at all **/
};

};  // namespace v3d::ui::immediate
