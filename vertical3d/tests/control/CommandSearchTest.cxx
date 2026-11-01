/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Router.h>
#include <api/log/Logger.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/component/SelectList.h>
#include <api/ui/component/TextBox.h>
#include <api/ui/component/menu/Bar.h>
#include <vertical3d/src/control/CommandSearch.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/pointer_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <entt/entt.hpp>

namespace {

using v3d::editor::control::CommandSearch;

/**
 * A ui with a menu bar of two menus, Create and View, and the search's panel. Create holds a
 * submenu of two meshes, a separator and Group. View holds Grid, and Cube a second time.
 **/
struct Searching {
    Searching() :
        dispatcher(boost::make_shared<entt::dispatcher>()),
        ui(boost::make_shared<v3d::ui::Interface>(boost::make_shared<v3d::event::Router>(dispatcher), dispatcher,
            boost::make_shared<v3d::log::Logger>())),
        search(ui) {
        BOOST_REQUIRE(ui->load(boost::json::parse(R"({ "themes": [], "containers": [
            { "name": "editor", "visible": true, "components": [ { "type": "menubar", "name": "menu", "menus": [
                { "label": "Create", "items": [
                    { "label": "Poly", "type": "submenu", "items": [
                        { "label": "Cube", "type": "action", "context": "create", "command": "poly::cube" },
                        { "label": "Cone", "type": "action", "context": "create", "command": "poly::cone" } ] },
                    { "type": "separator" },
                    { "label": "Group", "type": "action", "context": "create", "command": "group" } ] },
                { "label": "View", "items": [
                    { "label": "Grid", "type": "check", "context": "view", "command": "show::grid" },
                    { "label": "Cube", "type": "action", "context": "create", "command": "poly::cube" } ] } ] } ] },
            { "name": "search", "visible": false, "components": [
                { "type": "textbox", "name": "searchField", "context": "search", "command": "run" },
                { "type": "list", "name": "searchList", "context": "search", "command": "run" } ] }
        ] })").as_object()));
        bar = boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(ui->container("editor")->get("menu"));
        BOOST_REQUIRE(bar);
    }

    boost::shared_ptr<v3d::ui::component::TextBox> field() const {
        return boost::dynamic_pointer_cast<v3d::ui::component::TextBox>(ui->container("search")->get("searchField"));
    }

    boost::shared_ptr<v3d::ui::component::SelectList> list() const {
        return boost::dynamic_pointer_cast<v3d::ui::component::SelectList>(ui->container("search")->get("searchList"));
    }

    boost::shared_ptr<entt::dispatcher> dispatcher;
    boost::shared_ptr<v3d::ui::Interface> ui;
    boost::shared_ptr<v3d::ui::component::menu::Bar> bar;
    CommandSearch search;
};

/**
 * @return entries with these paths, each sending a command named for its place
 **/
std::vector<CommandSearch::Entry> entries(const std::vector<std::string>& paths) {
    std::vector<CommandSearch::Entry> made;
    made.reserve(paths.size());
    for (std::size_t index = 0; index < paths.size(); index++) {
        made.push_back({ .command = "test::" + std::to_string(index), .path = paths[index], .shortcut = {} });
    }
    return made;
}

BOOST_AUTO_TEST_SUITE(command_search_test)

/**
 * The catalogue lists each item that sends a command under the labels that lead to it, once,
 * and leaves out separators and submenus.
 **/
BOOST_FIXTURE_TEST_CASE(the_catalogue_is_the_menus_commands, Searching) {
    const std::vector<CommandSearch::Entry> found = CommandSearch::catalogue({ bar, nullptr });
    BOOST_REQUIRE_EQUAL(found.size(), 4U);
    BOOST_CHECK_EQUAL(found[0].command, "create::poly::cube");
    BOOST_CHECK_EQUAL(found[0].path, "Create > Poly > Cube");
    BOOST_CHECK_EQUAL(found[1].path, "Create > Poly > Cone");
    BOOST_CHECK_EQUAL(found[2].path, "Create > Group");
    BOOST_CHECK_EQUAL(found[3].command, "view::show::grid");
    BOOST_CHECK_EQUAL(found[3].path, "View > Grid");
}

/**
 * Every word has to be in the path. An entry whose own label begins with the first word comes
 * first, then one where every word begins a word, then the rest.
 **/
BOOST_AUTO_TEST_CASE(matches_are_ranked) {
    const std::vector<CommandSearch::Entry> all = entries({ "View > Show > Grid", "Create > Light > Spot", "Select Mask > Vertex",
        "Create > Poly > Cube", "Edit > Redo" });
    BOOST_CHECK((CommandSearch::match(all, "") == std::vector<std::size_t>{ 0, 1, 2, 3, 4 }));
    BOOST_CHECK((CommandSearch::match(all, "CUBE") == std::vector<std::size_t>{ 3 }));
    // Redo's label begins "re", and Create has it inside a word
    BOOST_CHECK((CommandSearch::match(all, "re") == std::vector<std::size_t>{ 4, 1, 3 }));
    // Spot's label does not begin "light", but both words begin words of its path
    BOOST_CHECK((CommandSearch::match(all, "light sp") == std::vector<std::size_t>{ 1 }));
    BOOST_CHECK((CommandSearch::match(all, "show sh") == std::vector<std::size_t>{ 0 }));
    BOOST_CHECK(CommandSearch::match(all, "cube grid").empty());
}

/**
 * A row is the path, and the shortcut after a gap when there is one.
 **/
BOOST_AUTO_TEST_CASE(a_row_shows_the_shortcut) {
    BOOST_CHECK_EQUAL(CommandSearch::row({ .command = "edit::undo", .path = "Edit > Undo", .shortcut = {} }), "Edit > Undo");
    const std::string row = CommandSearch::row({ .command = "edit::undo", .path = "Edit > Undo", .shortcut = "Ctrl+Z" });
    BOOST_CHECK(row.starts_with("Edit > Undo "));
    BOOST_CHECK(row.ends_with(" Ctrl+Z"));
}

/**
 * Opening shows the panel with every command listed, the first chosen and the keyboard in an
 * emptied field. Typing matches again, and closing hides the panel.
 **/
BOOST_FIXTURE_TEST_CASE(typing_lists_what_matches, Searching) {
    field()->text("left over");
    search.open(CommandSearch::catalogue({ bar }));
    BOOST_CHECK(search.visible());
    BOOST_CHECK(ui->container("search")->visible());
    BOOST_CHECK(ui->focused() == field());
    BOOST_CHECK(field()->text().empty());
    BOOST_CHECK_EQUAL(list()->items().size(), 4U);
    BOOST_CHECK_EQUAL(list()->selected(), 0);
    BOOST_CHECK(search.chosen() == std::optional<std::string>("create::poly::cube"));

    field()->text("gro");
    search.sync();
    BOOST_CHECK_EQUAL(search.query(), "gro");
    BOOST_REQUIRE_EQUAL(list()->items().size(), 1U);
    BOOST_CHECK_EQUAL(list()->items()[0], "Create > Group");
    BOOST_CHECK(search.chosen() == std::optional<std::string>("create::group"));

    field()->text("nothing like it");
    search.sync();
    BOOST_CHECK(list()->items().empty());
    BOOST_CHECK(!search.chosen());

    search.close();
    BOOST_CHECK(!search.visible());
    BOOST_CHECK(!ui->container("search")->visible());
}

/**
 * The arrows move the choice and wrap at either end, and a row picked in the list is the one
 * chosen.
 **/
BOOST_FIXTURE_TEST_CASE(the_choice_moves_and_wraps, Searching) {
    search.open(CommandSearch::catalogue({ bar }));
    search.step(1);
    BOOST_CHECK(search.chosen() == std::optional<std::string>("create::poly::cone"));
    BOOST_CHECK_EQUAL(list()->selected(), 1);
    search.step(-2);
    BOOST_CHECK(search.chosen() == std::optional<std::string>("view::show::grid"));
    search.step(1);
    BOOST_CHECK(search.chosen() == std::optional<std::string>("create::poly::cube"));

    list()->selected(2);
    BOOST_CHECK(search.chosen() == std::optional<std::string>("create::group"));
    search.step(1);
    BOOST_CHECK(search.chosen() == std::optional<std::string>("view::show::grid"));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
