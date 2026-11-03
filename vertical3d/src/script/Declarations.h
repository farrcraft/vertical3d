/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <map>
#include <string>
#include <vector>

#include "Arguments.h"

namespace v3d::editor::script {

/**
 * The TypeScript declarations of the commands a script can run, which the editor ships as
 * data/scripts/editor.d.ts for an author's editor and tsc to check a script against.
 *
 * Each command is a function in the namespace of its path, such as timeline.set in the
 * timeline namespace. A command that takes arguments takes one object of them, every one
 * optional. A boolean is boolean, a number or an integer is number, text and an item are
 * string, and items and parts are string[]. console.log is declared too.
 *
 * @param commands each command a script can run, by name, with its parameters
 **/
std::string declarations(const std::map<std::string, std::vector<Parameter>>& commands);

};  // namespace v3d::editor::script
