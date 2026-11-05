/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Immediate.h>
#include <api/ui/tests/Typeset.h>
#include <vertical3d/src/control/ConsoleEditor.h>
#include <vertical3d/src/script/Journal.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::control::ConsoleEditor;
using v3d::editor::script::Journal;
using Written = v3d::ui::typeset::Written;

/**
 * The placeholder an empty line shows.
 **/
const char PROMPT[] = "Type a script, and press Return";

/**
 * A console over a journal, drawn into a 400 by 300 region at the canvas's corner, which keeps
 * each line it is asked to run. The layer measures ten pixels a character and records what it
 * writes.
 **/
struct Console {
    Console() :
        journal(boost::make_shared<Journal>()),
        layer(v3d::ui::typeset::mono(&written)),
        console(journal, [this](const std::string& line) { ran.push_back(line); }) {
        canvas.resize(glm::uvec2(500, 400));
    }

    /**
     * Draw one frame of the console under some input.
     **/
    void frame(const v3d::ui::Immediate::Input& input) {
        written.clear();
        layer.begin(&canvas, input);
        console.draw(&layer, 1, glm::vec4(0.0F, 0.0F, 400.0F, 300.0F));
        layer.end();
    }

    /**
     * @return where a text was written, the first of it
     **/
    std::optional<glm::vec2> at(const std::string& text) const {
        for (const Written& each : written) {
            if (each.text == text) {
                return each.pen;
            }
        }
        return std::nullopt;
    }

    static v3d::ui::Immediate::Input input(const glm::vec2& cursor, bool down, bool pressed, bool released, float wheel = 0.0F) {
        v3d::ui::Immediate::Input made;
        made.cursor = cursor;
        made.down = down;
        made.pressed = pressed;
        made.released = released;
        made.wheel = wheel;
        return made;
    }

    /**
     * Click the line, so it takes the keyboard.
     **/
    void focus() {
        frame({});
        const std::optional<glm::vec2> prompt = at(PROMPT);
        BOOST_REQUIRE(prompt);
        const glm::vec2 on = *prompt + glm::vec2(4.0F, -4.0F);
        frame(input(on, false, false, false));
        frame(input(on, true, true, false));
        frame(input(on, false, false, true));
        BOOST_REQUIRE(layer.typing());
    }

    /**
     * Type a line and press return, then draw a frame.
     **/
    void enter(const std::string& line) {
        BOOST_REQUIRE(layer.typing());
        BOOST_CHECK(layer.typed(line));
        frame({});
        BOOST_CHECK(layer.press("return"));
        frame({});
    }

    boost::shared_ptr<Journal> journal;
    v3d::render::realtime::canvas::Canvas canvas;
    std::vector<Written> written;
    v3d::ui::Immediate layer;
    std::vector<std::string> ran;
    ConsoleEditor console;
};

BOOST_AUTO_TEST_SUITE(console_editor_test)

/**
 * The journal is listed newest at the foot, a typed line after a >, and an entry over lines as
 * a row for each.
 **/
BOOST_FIXTURE_TEST_CASE(the_journal_is_listed_newest_at_the_foot, Console) {
    journal->add(Journal::Kind::Typed, "edit.undo()");
    journal->add(Journal::Kind::Command, "edit.undo()");
    journal->add(Journal::Kind::Said, "first\nsecond");
    frame({});
    // each row is written, and each lower than the one before it
    std::vector<float> heights;
    for (const char* row : { "> edit.undo()", "edit.undo()", "first", "second", PROMPT }) {
        BOOST_CHECK_MESSAGE(at(row), row);
        heights.push_back(at(row).value_or(glm::vec2(0.0F)).y);
    }
    for (std::size_t each = 1; each < heights.size(); ++each) {
        BOOST_CHECK_LT(heights[each - 1], heights[each]);
    }
}

/**
 * Return runs the line, empties it, and keeps the keyboard for the next one. An empty line
 * runs nothing.
 **/
BOOST_FIXTURE_TEST_CASE(return_runs_the_line_and_keeps_the_keyboard, Console) {
    focus();
    enter("create.poly.cube()");
    BOOST_CHECK(ran == std::vector<std::string>{ "create.poly.cube()" });
    BOOST_CHECK_EQUAL(console.line(1), "");
    frame({});
    BOOST_CHECK(layer.typing());
    BOOST_CHECK(console.typing());

    enter("   ");
    BOOST_CHECK_EQUAL(ran.size(), 1U);
    frame({});
    BOOST_CHECK(layer.typing());
}

/**
 * The arrows step through the lines run before, oldest last, and past the newest back to the
 * line being written.
 **/
BOOST_FIXTURE_TEST_CASE(the_arrows_step_through_lines_run_before, Console) {
    focus();
    enter("first()");
    frame({});
    enter("second()");
    frame({});
    BOOST_CHECK(layer.typed("draft"));
    frame({});

    console.recall(-1);
    BOOST_CHECK_EQUAL(console.line(1), "second()");
    console.recall(-1);
    BOOST_CHECK_EQUAL(console.line(1), "first()");
    console.recall(-1);
    BOOST_CHECK_EQUAL(console.line(1), "first()");
    console.recall(1);
    BOOST_CHECK_EQUAL(console.line(1), "second()");
    console.recall(1);
    BOOST_CHECK_EQUAL(console.line(1), "draft");

    // a recalled line is what the field then edits
    console.recall(-1);
    frame({});
    BOOST_CHECK(layer.typed("!"));
    frame({});
    BOOST_CHECK_EQUAL(console.line(1), "second()!");
}

/**
 * The arrows do nothing while no console's line holds the keyboard.
 **/
BOOST_FIXTURE_TEST_CASE(the_arrows_need_the_keyboard, Console) {
    focus();
    enter("first()");
    // the line takes the keyboard back as it is drawn after the return
    frame({});
    BOOST_REQUIRE(console.typing());
    BOOST_CHECK(layer.press("escape"));
    frame({});
    BOOST_CHECK(!console.typing());
    console.recall(-1);
    BOOST_CHECK_EQUAL(console.line(1), "");
}

/**
 * The wheel scrolls back through the list, and an entry added brings it to the foot again.
 **/
BOOST_FIXTURE_TEST_CASE(the_wheel_scrolls_back, Console) {
    for (int each = 0; each < 40; ++each) {
        journal->add(Journal::Kind::Said, "row " + std::to_string(each));
    }
    frame({});
    BOOST_CHECK(at("row 39"));
    BOOST_CHECK(!at("row 0"));
    const glm::vec2 over(100.0F, 40.0F);
    frame(input(over, false, false, false));
    for (int notch = 0; notch < 20; ++notch) {
        frame(input(over, false, false, false, 1.0F));
    }
    BOOST_CHECK(at("row 0"));
    BOOST_CHECK(!at("row 39"));

    journal->add(Journal::Kind::Said, "row 40");
    frame({});
    BOOST_CHECK(at("row 40"));
    BOOST_CHECK(!at("row 0"));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
