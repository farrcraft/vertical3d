/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/database/Statement.h>

namespace v3d::editor {

/**
 * The two inserts a write runs for each item.
 **/
struct Inserts {
    v3d::database::Statement item;
    v3d::database::Statement buffer;
};

};  // namespace v3d::editor
