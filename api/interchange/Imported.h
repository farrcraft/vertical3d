/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Scene.h"

namespace v3d::interchange {

/**
 * What a reader made of a file.
 *
 * scene is empty when the file could not be read, and problem then says why. notes lists what
 * the file held that the scene cannot describe and was left out, such as a skin or a mesh of
 * lines, one line each.
 **/
struct Imported {
    std::optional<Scene> scene;
    std::string problem;
    std::vector<std::string> notes;
};

};  // namespace v3d::interchange
