/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <vertical3d/src/command/Show.h>
#include <vertical3d/src/scene/item/Group.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/Scene.h>

#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

using v3d::editor::command::Show;

/**
 * A scene of a group holding a mesh, and a mesh beside the group.
 **/
struct Held final {
    boost::shared_ptr<v3d::editor::scene::Scene> scene = boost::make_shared<v3d::editor::scene::Scene>();
    boost::shared_ptr<v3d::editor::scene::item::Group> holder = boost::make_shared<v3d::editor::scene::item::Group>();
    boost::shared_ptr<v3d::editor::scene::item::Mesh> inside = boost::make_shared<v3d::editor::scene::item::Mesh>(boost::make_shared<v3d::brep::BRep>());
    boost::shared_ptr<v3d::editor::scene::item::Mesh> beside = boost::make_shared<v3d::editor::scene::item::Mesh>(boost::make_shared<v3d::brep::BRep>());

    Held() {
        scene->add(holder);
        scene->add(inside);
        scene->add(beside);
        scene->parent(inside->id(), holder->id());
    }
};

BOOST_AUTO_TEST_SUITE(show_test)

/**
 * A change sets each item's flags, and undo sets them back. A change that leaves an item's flags
 * as they are is dropped, and a command of only those changes nothing.
 **/
BOOST_AUTO_TEST_CASE(a_change_sets_the_flags_and_undo_puts_them_back) {
    const Held held;
    Show none(held.scene, { { .item = held.beside, .shown = { .views = true, .renders = true } } }, "show");
    BOOST_CHECK(none.empty());

    Show change(held.scene, { { .item = held.beside, .shown = { .views = true, .renders = false } } }, "shown in renders");
    BOOST_CHECK(!change.empty());
    BOOST_CHECK_EQUAL(change.name(), "shown in renders");
    change.redo();
    BOOST_CHECK(held.beside->shown() == (v3d::editor::scene::item::Item::Shown{ .views = true, .renders = false }));
    change.undo();
    BOOST_CHECK(held.beside->shown() == v3d::editor::scene::item::Item::Shown{});
}

/**
 * Hiding an item deselects what it hides, its children included, and undo selects them again.
 * What it does not hide stays selected.
 **/
BOOST_AUTO_TEST_CASE(what_is_hidden_is_deselected) {
    const Held held;
    held.inside->selected(true);
    held.beside->selected(true);

    Show hide(held.scene, { { .item = held.holder, .shown = { .views = false, .renders = true } } }, "hide");
    hide.redo();
    BOOST_CHECK(!held.scene->shown(held.inside->id()).views);
    BOOST_CHECK(!held.inside->selected());
    BOOST_CHECK(held.beside->selected());

    hide.undo();
    BOOST_CHECK(held.scene->shown(held.inside->id()).views);
    BOOST_CHECK(held.inside->selected());
    BOOST_CHECK(held.beside->selected());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
