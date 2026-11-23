/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <MaterialXCore/Document.h>
#include <MaterialXCore/Node.h>

#include <algorithm>
#include <string>
#include <vector>

namespace v3d::moya::material::internal {

/** A material's network as MaterialX holds it, and what reading it left out and why. **/
struct Network {
    MaterialX::DocumentPtr document;

    /** The node of the material's terminal being compiled, or none. **/
    MaterialX::NodePtr terminal;

    std::vector<std::string> reports;

    /** Reports a message, once however often it is given. **/
    void report(const std::string & message) {
        if (std::find(reports.begin(), reports.end(), message) == reports.end()) {
            reports.push_back(message);
        }
    }
};

};  // namespace v3d::moya::material::internal
