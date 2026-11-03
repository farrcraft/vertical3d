/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/script/Runtime.h>
#include <api/script/Strip.h>

#include <chrono>
#include <string>
#include <vector>

#include <boost/json.hpp>
#include <boost/test/unit_test.hpp>

namespace {

using v3d::script::Problem;
using v3d::script::Result;
using v3d::script::Runtime;

/**
 * @return what a run reported as wrong, or an empty problem when nothing was
 **/
Problem problemOf(const Result& result) {
    return result.problem.value_or(Problem{});
}

BOOST_AUTO_TEST_SUITE(script_runtime_test)

/**
 * A run's value is its last statement's, as a person reads it: a string in quotes, an object as
 * JSON, and nothing for undefined.
 **/
BOOST_AUTO_TEST_CASE(a_run_gives_its_last_value) {
    Runtime runtime;
    BOOST_CHECK_EQUAL(runtime.run("1 + 2", "console").value, "3");
    BOOST_CHECK_EQUAL(runtime.run("'a' + 'b'", "console").value, "\"ab\"");
    BOOST_CHECK_EQUAL(runtime.run("({ a: [1, true, null] })", "console").value, R"({"a":[1,true,null]})");
    BOOST_CHECK_EQUAL(runtime.run("let nothing = 1", "console").value, "");
    BOOST_CHECK_EQUAL(runtime.run("undefined", "console").value, "");
    BOOST_CHECK(!runtime.run("1", "console").problem);
}

/**
 * What one run declares, the next can use, and a run's types are stripped before it runs.
 **/
BOOST_AUTO_TEST_CASE(runs_share_one_scope) {
    Runtime runtime;
    BOOST_CHECK(!runtime.run("let count: number = 2;\nfunction twice(x: number): number { return x * 2; }", "first").problem);
    BOOST_CHECK_EQUAL(runtime.run("twice(count) as number", "second").value, "4");
}

/**
 * A bound function is reached by its path, with its arguments as JSON, and its value returns.
 **/
BOOST_AUTO_TEST_CASE(a_bound_function_is_called_by_its_path) {
    Runtime runtime;
    std::vector<boost::json::array> calls;
    BOOST_CHECK(runtime.bind("timeline.set", [&calls](const boost::json::array& arguments) {
        calls.push_back(arguments);
        return boost::json::value(42);
    }));
    BOOST_CHECK(runtime.bind("timeline.play", [](const boost::json::array&) { return boost::json::value("playing"); }));
    BOOST_CHECK_EQUAL(runtime.run("timeline.set({ items: ['Cube'], frame: 24 }, undefined, () => 1)", "console").value, "42");
    BOOST_REQUIRE_EQUAL(calls.size(), 1U);
    BOOST_CHECK_EQUAL(boost::json::serialize(calls.front()), R"([{"items":["Cube"],"frame":24},null,null])");
    BOOST_CHECK_EQUAL(runtime.run("timeline.play()", "console").value, "\"playing\"");
}

/**
 * A path that is empty, holds an empty name, or passes through something that is not an object
 * binds nothing.
 **/
BOOST_AUTO_TEST_CASE(a_path_that_cannot_hold_a_function_is_refused) {
    Runtime runtime;
    const auto nothing = [](const boost::json::array&) { return boost::json::value(); };
    BOOST_CHECK(!runtime.bind("", nothing));
    BOOST_CHECK(!runtime.bind("a..b", nothing));
    BOOST_CHECK(!runtime.bind("a.", nothing));
    BOOST_CHECK(!runtime.run("var number = 1", "console").problem);
    BOOST_CHECK(!runtime.bind("number.b", nothing));
    BOOST_CHECK(runtime.bind("a", nothing));
}

/**
 * A function that refuses a call throws an Error the script can catch, with its message.
 **/
BOOST_AUTO_TEST_CASE(a_refusal_is_an_error_in_the_script) {
    Runtime runtime;
    BOOST_CHECK(runtime.bind("edit.key", [](const boost::json::array&) -> boost::json::value {
        throw v3d::script::Refused("set key: there is no item called Sphere");
    }));
    BOOST_CHECK_EQUAL(runtime.run("try { edit.key(); } catch (e) { e.message }", "console").value,
        "\"set key: there is no item called Sphere\"");
    const Problem problem = problemOf(runtime.run("\n  edit.key();", "console"));
    BOOST_CHECK_EQUAL(problem.message, "Error: set key: there is no item called Sphere");
    BOOST_CHECK_EQUAL(problem.line, 2U);
}

/**
 * An error a script throws is reported with where it arose in the source, and so is one the
 * engine finds reading it.
 **/
BOOST_AUTO_TEST_CASE(an_error_names_where_it_arose) {
    Runtime runtime;
    const Problem thrown = problemOf(runtime.run("let a = 1;\nlet b = 2;\n    missing();", "console"));
    BOOST_CHECK_EQUAL(thrown.message, "ReferenceError: missing is not defined");
    BOOST_CHECK_EQUAL(thrown.line, 3U);
    BOOST_CHECK_EQUAL(thrown.column, 5U);

    // a source that does not read is found by the stripper, and one that reads but breaks a rule
    // of the language by the engine
    const Problem unread = problemOf(runtime.run("let ok = 1;\nlet x = {;", "console"));
    BOOST_CHECK_EQUAL(unread.message, "the script cannot be read here");
    BOOST_CHECK_EQUAL(unread.line, 2U);
    const Problem syntax = problemOf(runtime.run("let twice = 1;\nlet twice = 2;", "console"));
    BOOST_CHECK(syntax.message.starts_with("SyntaxError"));
    BOOST_CHECK_EQUAL(syntax.line, 2U);

    const Problem plain = problemOf(runtime.run("throw 'just text'", "console"));
    BOOST_CHECK_EQUAL(plain.message, "uncaught \"just text\"");
}

/**
 * A source that cannot be stripped does not run, and says where and why.
 **/
BOOST_AUTO_TEST_CASE(a_source_that_cannot_be_stripped_does_not_run) {
    Runtime runtime;
    int calls = 0;
    BOOST_CHECK(runtime.bind("touch", [&calls](const boost::json::array&) {
        calls++;
        return boost::json::value();
    }));
    const Problem problem = problemOf(runtime.run("touch();\nenum E { A }", "console"));
    BOOST_CHECK(problem.message.starts_with("an enum cannot be erased"));
    BOOST_CHECK_EQUAL(problem.line, 2U);
    BOOST_CHECK_EQUAL(calls, 0);
}

/**
 * A run past its time limit is stopped, and the runtime runs again after it.
 **/
BOOST_AUTO_TEST_CASE(a_run_past_its_time_is_stopped) {
    Runtime runtime;
    runtime.limit(std::chrono::milliseconds(50));
    const Problem problem = problemOf(runtime.run("for (;;) {}", "console"));
    BOOST_CHECK(problem.message.starts_with("the script was stopped"));
    BOOST_CHECK_EQUAL(runtime.run("2 * 3", "console").value, "6");
}

/**
 * A run that recurses without end is stopped at the runtime's stack limit.
 **/
BOOST_AUTO_TEST_CASE(a_run_past_its_stack_is_stopped) {
    Runtime runtime;
    const Problem problem = problemOf(runtime.run("function down(n) { return down(n + 1) + 1; } down(0)", "console"));
    BOOST_CHECK(problem.message.starts_with("RangeError") || problem.message.starts_with("InternalError") ||
        problem.message.starts_with("the script was stopped"));
    BOOST_CHECK_EQUAL(runtime.run("'still here'", "console").value, "\"still here\"");
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
