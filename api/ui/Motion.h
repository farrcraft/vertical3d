/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/ui/Opening.h>
#include <api/ui/Transition.h>

namespace v3d::ui {

/**
 * What a retained component or an immediate widget keeps of its transitions between frames.
 * Layout never reads it.
 **/
struct Motion final {
    Transition face;  /**< the fill behind it **/
    Transition ink;   /**< its text **/
    Transition shortcut;  /**< the shortcut a menu item shows beside its text **/
    Opening opening;  /**< how far it has opened, for what drops or pops up **/
};

};  // namespace v3d::ui
