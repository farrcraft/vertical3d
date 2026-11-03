/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vertical3d/src/script/Arguments.h>
#include <vertical3d/src/script/Declarations.h>

#include <map>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace {

using v3d::editor::script::Parameter;
using v3d::editor::script::Type;

BOOST_AUTO_TEST_SUITE(script_declarations_test)

/**
 * Each command is a function in the namespace of its path, taking one optional object of its
 * arguments, each optional, in TypeScript's types. console.log is declared beside them.
 **/
BOOST_AUTO_TEST_CASE(each_command_is_declared_in_its_namespace) {
    const std::map<std::string, std::vector<Parameter>> commands{
        { "edit::undo", {} },
        { "timeline::set", { { "items", Type::Items }, { "frame", Type::Integer } } },
        { "timeline::auto_key", { { "on", Type::Boolean } } },
        { "transform::poly::inset_face", { { "parts", Type::Parts } } },
        { "a::b", { { "amount", Type::Number }, { "label", Type::Text }, { "item", Type::Item } } } };
    BOOST_CHECK_EQUAL(v3d::editor::script::declarations(commands),
        "// The commands a script can run in the editor. The editor writes this file from its own\n"
        "// commands, and a test checks the two agree.\n"
        "\n"
        "declare namespace a {\n"
        "    function b(args?: { amount?: number; label?: string; item?: string }): void;\n"
        "}\n"
        "\n"
        "declare namespace console {\n"
        "    function log(...values: unknown[]): void;\n"
        "}\n"
        "\n"
        "declare namespace edit {\n"
        "    function undo(): void;\n"
        "}\n"
        "\n"
        "declare namespace timeline {\n"
        "    function auto_key(args?: { on?: boolean }): void;\n"
        "    function set(args?: { items?: string[]; frame?: number }): void;\n"
        "}\n"
        "\n"
        "declare namespace transform.poly {\n"
        "    function inset_face(args?: { parts?: string[] }): void;\n"
        "}\n");
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
