/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Json.h>
#include <api/event/Commands.h>
#include <api/event/Event.h>
#include <api/log/Logger.h>
#include <vertical3d/src/app/AreaCommands.h>
#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/app/EditCommands.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/item/Components.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/script/Arguments.h>
#include <vertical3d/src/script/Journal.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/tool/Select.h>
#include <vertical3d/src/view/AreaLayout.h>

#include <optional>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <boost/test/unit_test.hpp>

namespace {

using v3d::editor::script::Journal;
using v3d::editor::script::Type;

/**
 * A command table over the engine's registry, counting the read backs that follow a command.
 **/
struct Table {
    Table() :
        table(&commands, [this]() { after++; }) {
    }

    /**
     * @return the text of each journal entry, oldest first
     **/
    std::vector<std::string> journal() const {
        std::vector<std::string> texts;
        for (const Journal::Entry& entry : table.journal()->entries()) {
            texts.push_back(entry.text);
        }
        return texts;
    }

    v3d::event::Commands commands;
    v3d::editor::app::CommandTable table;
    int after = 0;
};

/**
 * The edit commands over a document holding a cube, in a project in the temporary directory
 * that is removed before and after the case.
 **/
struct Edits : Table {
    Edits() :
        project(boost::filesystem::temp_directory_path() / "v3d_command_table"),
        logger(boost::make_shared<v3d::log::Logger>()) {
        boost::filesystem::remove_all(project);
        context.logger = logger;
        context.document = boost::make_shared<v3d::editor::document::Document>(logger, project);
        context.selectTool = boost::make_shared<v3d::editor::tool::Select>(context.document->scene(), logger);
        context.journal = table.journal();
        v3d::editor::app::EditCommands(context).registerCommands(&table);
        context.document->createPrimitive(v3d::editor::document::Document::Primitive::Cube);
        cube = context.document->scene()->named("Cube");
        BOOST_REQUIRE(cube);
    }

    ~Edits() {
        boost::system::error_code error;
        boost::filesystem::remove_all(project, error);
    }

    Edits(const Edits&) = delete;
    Edits& operator=(const Edits&) = delete;

    boost::filesystem::path project;
    boost::shared_ptr<v3d::log::Logger> logger;
    v3d::editor::app::Context context;
    boost::shared_ptr<v3d::editor::scene::item::Item> cube;
};

BOOST_AUTO_TEST_SUITE(command_table_test)

/**
 * A command a script can run is run by a key with nothing given, fills what it takes, and is
 * listed as the line that runs it again with what it filled. The read back follows it.
 **/
BOOST_FIXTURE_TEST_CASE(a_key_runs_a_command_and_lists_its_line, Table) {
    int ran = 0;
    table.script("timeline::set", { { "items", Type::Items }, { "frame", Type::Integer } }, [&ran](v3d::editor::script::Arguments* arguments) {
        arguments->integer("frame", []() { return 24; });
        ran++;
    });
    BOOST_CHECK(commands.invoke(v3d::event::Event("timeline::set")));
    BOOST_CHECK_EQUAL(ran, 1);
    BOOST_CHECK_EQUAL(after, 1);
    BOOST_CHECK(journal() == std::vector<std::string>{ "timeline.set({ frame: 24 })" });
    BOOST_CHECK(table.scripted().at("timeline::set").size() == 2U);
}

/**
 * A script runs a command with what it gives, and a command it cannot run, or an argument that
 * does not suit, is refused without running anything.
 **/
BOOST_FIXTURE_TEST_CASE(a_script_runs_a_command_with_its_arguments, Table) {
    std::optional<int> frame;
    table.script("timeline::set", { { "frame", Type::Integer } }, [&frame](v3d::editor::script::Arguments* arguments) {
        frame = arguments->integer("frame", []() { return 1; });
    });
    table.press("search::open", []() {});

    BOOST_CHECK(!table.run("timeline::set", boost::json::parse(R"({"frame": 7})").as_object()));
    BOOST_CHECK_EQUAL(frame.value_or(0), 7);
    BOOST_CHECK_EQUAL(after, 1);

    frame.reset();
    BOOST_CHECK_EQUAL(table.run("timeline::set", boost::json::parse(R"({"frame": "7"})").as_object()).value_or(""),
        "timeline.set: frame is a whole number");
    BOOST_CHECK_EQUAL(table.run("search::open", {}).value_or(""), "no command a script can run is called search.open");
    BOOST_CHECK_EQUAL(table.run("nothing::here", {}).value_or(""), "no command a script can run is called nothing.here");
    BOOST_CHECK(!frame);
    BOOST_CHECK_EQUAL(after, 1);
    BOOST_CHECK(journal() == std::vector<std::string>{ "timeline.set({ frame: 7 })" });
}

/**
 * A name registered a second time is refused, as a press's is, and the first stays.
 **/
BOOST_FIXTURE_TEST_CASE(a_second_script_of_a_name_is_refused, Table) {
    int first = 0;
    table.script("edit::undo", [&first]() { first++; });
    table.script("edit::undo", []() { BOOST_ERROR("the second handler ran"); });
    table.press("edit::redo", []() {});
    table.script("edit::redo", []() {});
    BOOST_CHECK(table.refused() == (std::vector<std::string>{ "edit::undo", "edit::redo" }));
    BOOST_CHECK(!table.run("edit::undo", {}));
    BOOST_CHECK_EQUAL(first, 1);
    BOOST_CHECK(!table.scripted().contains("edit::redo"));
}

/**
 * Set Key from a key keys the selected items at the current frame, and is listed with both, then
 * what it said. From a script it keys the items it names at the frame it names.
 **/
BOOST_FIXTURE_TEST_CASE(set_key_takes_its_items_and_frame, Edits) {
    cube->selected(true);
    BOOST_CHECK(commands.invoke(v3d::event::Event("timeline::set")));
    const std::vector<std::string> keyed = journal();
    BOOST_REQUIRE_GE(keyed.size(), 2U);
    BOOST_CHECK_EQUAL(keyed[keyed.size() - 2], R"(timeline.set({ items: ["Cube"], frame: 1 }))");
    BOOST_CHECK(table.journal()->entries().back().kind == Journal::Kind::Done);

    cube->selected(false);
    BOOST_CHECK(!table.run("timeline::set", boost::json::parse(R"({"items": ["Cube"], "frame": 24})").as_object()));
    const std::vector<float> frames = context.document->scene()->keyedFrames({ cube });
    BOOST_CHECK(frames == (std::vector<float>{ 1.0F, 24.0F }));

    // a refusal in a notice is the run's refusal too
    BOOST_CHECK_EQUAL(table.run("timeline::set", boost::json::parse(R"({"items": ["Sphere"]})").as_object()).value_or(""),
        "set key: there is no item called Sphere");
    BOOST_CHECK_EQUAL(table.journal()->entries().back().text, "set key: there is no item called Sphere");
    BOOST_CHECK(table.journal()->entries().back().kind == Journal::Kind::Refused);
}

/**
 * Auto key from a key turns it the other way, and is listed with the way it turned, so running
 * the line again sets it rather than flipping it.
 **/
BOOST_FIXTURE_TEST_CASE(auto_key_is_listed_with_its_state, Edits) {
    const bool was = context.document->scene()->timeline().autoKey;
    BOOST_CHECK(commands.invoke(v3d::event::Event("timeline::auto_key")));
    BOOST_CHECK_EQUAL(context.document->scene()->timeline().autoKey, !was);
    const std::string line = std::string("timeline.auto_key({ on: ") + (was ? "false" : "true") + " })";
    BOOST_CHECK_EQUAL(table.journal()->entries().front().text, line);

    BOOST_CHECK_EQUAL(table.run("timeline::auto_key", boost::json::parse(was ? R"({"on": false})" : R"({"on": true})").as_object()).value_or(""),
        was ? "auto key is already off" : "auto key is already on");
    BOOST_CHECK_EQUAL(context.document->scene()->timeline().autoKey, !was);
}

/**
 * A modelling operation from a key needs its select mask, and is listed with the parts it
 * worked on. From a script it selects the parts it names first, whatever the mask.
 **/
BOOST_FIXTURE_TEST_CASE(a_modelling_operation_takes_its_parts, Edits) {
    BOOST_REQUIRE(context.selectTool->mask() != v3d::editor::selection::SelectMask::Face);
    BOOST_CHECK(commands.invoke(v3d::event::Event("transform::poly::inset_face")));
    BOOST_CHECK(table.journal()->entries().back().kind == Journal::Kind::Refused);
    BOOST_CHECK_EQUAL(table.journal()->entries()[table.journal()->entries().size() - 2].text, "transform.poly.inset_face()");

    BOOST_CHECK(!table.run("transform::poly::inset_face", boost::json::parse(R"({"parts": ["Cube.f[0]"]})").as_object()));
    BOOST_CHECK(table.journal()->entries().back().kind == Journal::Kind::Done);
    BOOST_CHECK_EQUAL(table.journal()->entries()[table.journal()->entries().size() - 2].text, R"(transform.poly.inset_face({ parts: ["Cube.f[0]"] }))");
    BOOST_CHECK(cube->selected());

    // a change of mask clears the parts, so the face is chosen again under the new one
    context.selectTool->mask(v3d::editor::selection::SelectMask::Face);
    context.document->scene()->components().select(cube->id(), v3d::editor::scene::item::Component::Face, 0, true);
    BOOST_CHECK(commands.invoke(v3d::event::Event("transform::poly::extrude_face")));
    BOOST_CHECK_EQUAL(table.journal()->entries()[table.journal()->entries().size() - 2].text, R"(transform.poly.extrude_face({ parts: ["Cube.f[0]"] }))");

    BOOST_CHECK_EQUAL(table.run("transform::poly::extrude_face", boost::json::parse(R"({"parts": ["Cube.f[99]"]})").as_object()).value_or(""),
        "extrude face: Cube has no part 99 of the kind Cube.f[99] names");
    BOOST_CHECK_EQUAL(table.journal()->entries().back().text, "extrude face: Cube has no part 99 of the kind Cube.f[99] names");
}

/**
 * The + on a tabbed area's strip adds a tab of the kind chosen, which the area then shows. A
 * viewport added so starts from the perspective view.
 **/
BOOST_FIXTURE_TEST_CASE(a_tab_of_a_kind_is_added_to_an_area, Table) {
    const boost::shared_ptr<v3d::editor::view::AreaLayout> layout =
        boost::make_shared<v3d::editor::view::AreaLayout>(boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(layout->load(boost::json::parse(R"({"root": {"editors": [{"kind": "timeline"}]}})").as_object()));
    v3d::editor::app::Context context;
    context.layout = layout;
    int changes = 0;
    context.areasChanged = [&changes]() { changes++; };
    // the commands it registers call back into it, so it outlives them, as the editor's own does
    const v3d::editor::app::AreaCommands areas(context);
    areas.registerCommands(&table);

    v3d::event::Event add("area::tab::add::graph");
    add.data(static_cast<int>(layout->areas()[0].id));
    BOOST_REQUIRE(commands.invoke(add));
    const std::vector<v3d::editor::view::AreaLayout::Editor>& editors = layout->areas()[0].editors;
    BOOST_REQUIRE_EQUAL(editors.size(), 2U);
    BOOST_CHECK_EQUAL(editors[1].kind, "graph");
    BOOST_CHECK_EQUAL(layout->areas()[0].chosen, 1U);
    BOOST_CHECK_EQUAL(changes, 1);

    v3d::event::Event view("area::tab::add::viewport");
    view.data(static_cast<int>(layout->areas()[0].id));
    BOOST_REQUIRE(commands.invoke(view));
    BOOST_CHECK_EQUAL(v3d::asset::readString(layout->areas()[0].editor().state, "camera").value_or(""), "Perspective");
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
