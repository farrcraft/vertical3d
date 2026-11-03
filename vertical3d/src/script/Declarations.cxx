/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Declarations.h"

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "Arguments.h"
#include "Text.h"

namespace v3d::editor::script {

namespace {

/**
 * @return the TypeScript type a parameter takes
 **/
std::string_view typed(Type type) {
    switch (type) {
        case Type::Boolean:
            return "boolean";
        case Type::Number:
        case Type::Integer:
            return "number";
        case Type::Text:
        case Type::Item:
            return "string";
        case Type::Items:
        case Type::Parts:
            return "string[]";
    }
    return "unknown";
}

/**
 * @return a command's function, as its namespace declares it
 **/
std::string function(std::string_view name, const std::vector<Parameter>& parameters) {
    std::string listed;
    for (const Parameter& parameter : parameters) {
        listed += (listed.empty() ? "" : "; ") + parameter.name + "?: " + std::string(typed(parameter.type));
    }
    return "    function " + std::string(name) + "(" + (listed.empty() ? "" : "args?: { " + listed + " }") + "): void;\n";
}

};  // namespace

/**
 **/
std::string declarations(const std::map<std::string, std::vector<Parameter>>& commands) {
    // each namespace's functions, the namespaces in order
    std::map<std::string, std::string> spaces;
    spaces["console"] = "    function log(...values: unknown[]): void;\n";
    for (const auto& [command, parameters] : commands) {
        const std::string called = path(command);
        const std::size_t dot = called.rfind('.');
        const std::string space = dot == std::string::npos ? std::string() : called.substr(0, dot);
        spaces[space] += function(dot == std::string::npos ? called : called.substr(dot + 1), parameters);
    }
    std::string made =
        "// The commands a script can run in the editor. The editor writes this file from its own\n"
        "// commands, and a test checks the two agree.\n";
    for (const auto& [space, functions] : spaces) {
        made += "\ndeclare namespace ";
        made += space;
        made += " {\n";
        made += functions;
        made += "}\n";
    }
    return made;
}

};  // namespace v3d::editor::script
