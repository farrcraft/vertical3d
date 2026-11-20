/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

namespace v3d::editor::document {

/**
 * What an operation did, for whoever asked for it to say.
 **/
struct Outcome final {
    bool done = false;    /**< whether it did what it was asked, rather than refusing **/
    std::string message;  /**< what to tell the user, whether it changed or not **/
    bool undoable = false;  /**< whether it is a change undo takes back **/
};

};  // namespace v3d::editor::document
