/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

namespace v3d::engine {

/**
 * Who an app is: the names its player's files are kept under, in the directory userPath()
 * gives for them.
 **/
struct Identity final {
    /** The organization the app belongs to, the same for every app that shares it. **/
    std::string organization;
    /**
     * What the app is called. It is never changed once chosen, because changing it orphans
     * every player's settings. It also names the app in the log.
     **/
    std::string application;

    /** Whether the app has a name to keep a player's files under. **/
    bool named() const noexcept {
        return !organization.empty() && !application.empty();
    }
};

};  // namespace v3d::engine
