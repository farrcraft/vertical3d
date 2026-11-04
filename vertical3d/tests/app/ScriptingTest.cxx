/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Commands.h>
#include <api/log/Logger.h>
#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/app/EditCommands.h>
#include <vertical3d/src/app/Scripting.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/script/Journal.h>
#include <vertical3d/src/tool/Select.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <boost/test/unit_test.hpp>

namespace {

using v3d::editor::script::Journal;

/**
 * The edit commands bound into a runtime, over a document holding a cube, in a project in the
 * temporary directory that is removed before and after the case.
 **/
struct Console {
    Console() :
        table(&commands, []() {}),
        scripting(&table),
        project(boost::filesystem::temp_directory_path() / "v3d_scripting"),
        logger(boost::make_shared<v3d::log::Logger>()) {
        boost::filesystem::remove_all(project);
        context.logger = logger;
        context.document = boost::make_shared<v3d::editor::document::Document>(logger, project);
        context.selectTool = boost::make_shared<v3d::editor::tool::Select>(context.document->scene(), logger);
        context.journal = table.journal();
        v3d::editor::app::EditCommands(context).registerCommands(&table);
        scripting.bind();
        context.document->createPrimitive(v3d::editor::document::Document::Primitive::Cube);
        cube = context.document->scene()->named("Cube");
        BOOST_REQUIRE(cube);
    }

    ~Console() {
        boost::system::error_code error;
        boost::filesystem::remove_all(project, error);
    }

    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;

    /**
     * Run a line, and @return the kind and text of each entry it added to the journal
     **/
    std::vector<std::pair<Journal::Kind, std::string>> typed(const std::string& line) {
        const std::uint64_t before = table.journal()->last();
        scripting.line(line);
        std::vector<std::pair<Journal::Kind, std::string>> added;
        for (const Journal::Entry& entry : table.journal()->entries()) {
            if (entry.number > before) {
                added.emplace_back(entry.kind, entry.text);
            }
        }
        return added;
    }

    v3d::event::Commands commands;
    v3d::editor::app::CommandTable table;
    v3d::editor::app::Scripting scripting;
    boost::filesystem::path project;
    boost::shared_ptr<v3d::log::Logger> logger;
    v3d::editor::app::Context context;
    boost::shared_ptr<v3d::editor::scene::item::Item> cube;
};

using Added = std::vector<std::pair<Journal::Kind, std::string>>;

BOOST_AUTO_TEST_SUITE(scripting_test)

/**
 * A line runs the commands it calls, with the arguments it gives, and the journal lists the
 * line, each command and what it said.
 **/
BOOST_FIXTURE_TEST_CASE(a_line_runs_commands, Console) {
    const Added added = typed("for (const frame of [1, 24]) { timeline.set({ items: ['Cube'], frame }); }");
    BOOST_REQUIRE_EQUAL(added.size(), 5U);
    BOOST_CHECK(added[0].first == Journal::Kind::Typed);
    BOOST_CHECK_EQUAL(added[1].second, R"(timeline.set({ items: ["Cube"], frame: 1 }))");
    BOOST_CHECK(added[2].first == Journal::Kind::Done);
    BOOST_CHECK_EQUAL(added[3].second, R"(timeline.set({ items: ["Cube"], frame: 24 }))");
    BOOST_CHECK(context.document->scene()->keyedFrames({ cube }) == (std::vector<float>{ 1.0F, 24.0F }));
}

/**
 * A command that refuses stops the line with its refusal, as an Error the line could catch.
 **/
BOOST_FIXTURE_TEST_CASE(a_refusal_stops_the_line, Console) {
    const Added added = typed("timeline.set({ items: ['Sphere'] });\ntimeline.set({ items: ['Cube'] });");
    BOOST_REQUIRE_EQUAL(added.size(), 4U);
    BOOST_CHECK(added[2].first == Journal::Kind::Refused);
    BOOST_CHECK(added[3].first == Journal::Kind::Failed);
    BOOST_CHECK_EQUAL(added[3].second, "1:9 Error: set key: there is no item called Sphere");
    BOOST_CHECK(context.document->scene()->keyedFrames({ cube }).empty());

    const Added caught = typed("try { edit.undo(); edit.undo(); } catch (e) { console.log('caught', e.message) }");
    BOOST_REQUIRE(!caught.empty());
    BOOST_CHECK(caught.back().first == Journal::Kind::Said);
    BOOST_CHECK(caught.back().second.starts_with("caught "));
}

/**
 * Arguments that do not suit a command are refused before it runs.
 **/
BOOST_FIXTURE_TEST_CASE(arguments_that_do_not_suit_are_refused, Console) {
    BOOST_CHECK(typed("timeline.set({ frame: 'one' })").back().second.ends_with("Error: timeline.set: frame is a whole number"));
    BOOST_CHECK(typed("edit.undo(5)").back().second.ends_with("Error: edit.undo takes one object of arguments"));
    BOOST_CHECK(typed("edit.undo({}, {})").back().second.ends_with("Error: edit.undo takes one object of arguments"));
    BOOST_CHECK(typed("timeline.auto_key({ colour: 1 })").back().second.ends_with("Error: timeline.auto_key: it takes no argument called colour"));
}

/**
 * console.log lists what it is given, a string as it is and anything else as JSON, and a line
 * that comes to a value lists that.
 **/
BOOST_FIXTURE_TEST_CASE(what_a_line_says_is_listed, Console) {
    BOOST_CHECK(typed("console.log('frames', [1, 2], { a: true })") ==
        (Added{ { Journal::Kind::Typed, "console.log('frames', [1, 2], { a: true })" }, { Journal::Kind::Said, R"(frames [1,2] {"a":true})" } }));
    BOOST_CHECK(typed("let n: number = 6; n * 7") == (Added{ { Journal::Kind::Typed, "let n: number = 6; n * 7" }, { Journal::Kind::Said, "42" } }));
    BOOST_CHECK(typed("enum E { A }").back() == (std::pair<Journal::Kind, std::string>{ Journal::Kind::Failed,
        "1:1 an enum cannot be erased, so a script cannot hold one. An object of constants can stand in for it" }));
}

/**
 * Each example script the editor ships runs to its end on a document holding a cube, and runs at
 * least one command. A refusal it catches is allowed, and one it does not catch fails the case.
 **/
BOOST_AUTO_TEST_CASE(the_example_scripts_run) {
    std::vector<boost::filesystem::path> examples;
    boost::system::error_code error;
    for (boost::filesystem::directory_iterator it("data/scripts/examples", error), end; !error && it != end; it.increment(error)) {
        if (it->path().extension() == ".ts") {
            examples.push_back(it->path());
        }
    }
    std::sort(examples.begin(), examples.end());
    BOOST_REQUIRE(!examples.empty());
    for (const boost::filesystem::path& example : examples) {
        BOOST_TEST_CONTEXT(example.generic_string()) {
            std::ifstream file(example.string(), std::ios::binary);
            const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            BOOST_REQUIRE(!text.empty());
            Console console;
            const Added added = console.typed(text);
            BOOST_CHECK(std::none_of(added.begin(), added.end(), [](const auto& entry) { return entry.first == Journal::Kind::Failed; }));
            BOOST_CHECK(std::any_of(added.begin(), added.end(), [](const auto& entry) { return entry.first == Journal::Kind::Done; }));
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
