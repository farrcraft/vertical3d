/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/curve/Curve.h>
#include <api/curve/Curves.h>
#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/CreateItem.h>
#include <vertical3d/src/scene/Curve.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>
#include <vertical3d/src/store/Project.h>

#include <string>
#include <variant>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/system/error_code.hpp>
#include <boost/test/unit_test.hpp>

namespace {

/**
 * A project directory in the temporary directory, removed before and after the case.
 **/
class Scratch final {
 public:
    explicit Scratch(const std::string& name) :
        path_(boost::filesystem::temp_directory_path() / ("v3d_document_" + name)) {
        boost::filesystem::remove_all(path_);
    }

    ~Scratch() {
        boost::system::error_code error;
        boost::filesystem::remove_all(path_, error);
    }

    Scratch(const Scratch&) = delete;
    Scratch& operator=(const Scratch&) = delete;

    const boost::filesystem::path& path() const {
        return path_;
    }

 private:
    boost::filesystem::path path_;
};

v3d::editor::document::Document document(const boost::filesystem::path& project) {
    return v3d::editor::document::Document(boost::make_shared<v3d::log::Logger>(), project);
}

BOOST_AUTO_TEST_SUITE(document_test)

/**
 * An operation applies itself and records itself, so the change it made is one undo away, and
 * it says what it did. Undo and redo say what they stepped over.
 **/
BOOST_AUTO_TEST_CASE(an_operation_applies_and_records_itself) {
    const Scratch scratch("records");
    v3d::editor::document::Document edited = document(scratch.path());
    BOOST_CHECK(!edited.dirty());

    const v3d::editor::document::Outcome made = edited.createPrimitive(v3d::editor::document::Document::Primitive::Cube);
    BOOST_CHECK(made.done);
    BOOST_CHECK(made.undoable);
    BOOST_CHECK_EQUAL(made.message, "created a cube - 1 item");
    BOOST_CHECK_EQUAL(edited.scene()->count(), 1U);
    BOOST_CHECK_EQUAL(edited.scene()->items()[0]->name(), "Cube");
    BOOST_CHECK_EQUAL(edited.history()->undoDepth(), 1U);
    BOOST_CHECK(edited.dirty());

    // an undo is not itself undone, and a redo is
    const v3d::editor::document::Outcome undone = edited.undo();
    BOOST_CHECK(undone.done);
    BOOST_CHECK(!undone.undoable);
    BOOST_CHECK_EQUAL(undone.message, "undo create cube");
    BOOST_CHECK_EQUAL(edited.scene()->count(), 0U);
    BOOST_CHECK(!edited.dirty());

    const v3d::editor::document::Outcome redone = edited.redo();
    BOOST_CHECK(redone.done);
    BOOST_CHECK(redone.undoable);
    BOOST_CHECK_EQUAL(edited.scene()->count(), 1U);
    const v3d::editor::document::Outcome nothing = edited.redo();
    BOOST_CHECK(!nothing.done);
    BOOST_CHECK(!nothing.undoable);
}

/**
 * A curve is made at the origin, of the type asked for, as one change.
 **/
BOOST_AUTO_TEST_CASE(a_curve_is_created_as_one_change) {
    const Scratch scratch("curve");
    v3d::editor::document::Document edited = document(scratch.path());
    const v3d::editor::document::Outcome made = edited.createCurve(v3d::editor::scene::CurveType::Nurbs);
    BOOST_CHECK(made.done);
    BOOST_CHECK_EQUAL(made.message, "created a NURBS curve - 1 item");
    BOOST_REQUIRE_EQUAL(edited.scene()->count(), 1U);
    // a new item is named for its kind, each word begun in capitals
    BOOST_CHECK_EQUAL(edited.scene()->items()[0]->name(), "NURBS Curve");
    const boost::shared_ptr<v3d::editor::scene::Curve> curve = v3d::editor::scene::as<v3d::editor::scene::Curve>(edited.scene()->items()[0]);
    BOOST_REQUIRE(curve);
    BOOST_REQUIRE_EQUAL(curve->geometry()->size(), 1U);
    BOOST_CHECK(std::holds_alternative<v3d::curve::Nurbs>(curve->geometry()->curve(0).shape));
    BOOST_CHECK(!curve->style().renders);

    BOOST_CHECK(edited.undo().done);
    BOOST_CHECK_EQUAL(edited.scene()->count(), 0U);
}

/**
 * An operation that cannot run changes nothing, records nothing, and says why.
 **/
BOOST_AUTO_TEST_CASE(an_operation_that_cannot_run_says_why) {
    const Scratch scratch("refuses");
    v3d::editor::document::Document edited = document(scratch.path());

    const v3d::editor::document::Outcome grouped = edited.group({});
    BOOST_CHECK(!grouped.done);
    BOOST_CHECK(!grouped.message.empty());

    const v3d::editor::document::Outcome split = edited.splitEdges();
    BOOST_CHECK(!split.done);
    BOOST_CHECK_EQUAL(split.message, "split edge: no edge is selected");

    BOOST_CHECK(!edited.undo().done);
    BOOST_CHECK_EQUAL(edited.undo().message, "nothing to undo");
    BOOST_CHECK_EQUAL(edited.history()->undoDepth(), 0U);
    BOOST_CHECK(!edited.dirty());
}

/**
 * The items an operation acts on are its arguments, not what is selected. Settings the
 * document already has are no change.
 **/
BOOST_AUTO_TEST_CASE(an_operation_acts_on_what_it_is_given) {
    const Scratch scratch("arguments");
    v3d::editor::document::Document edited = document(scratch.path());
    const v3d::type::camera::Profile at("Perspective");
    edited.createCamera(at, v3d::editor::scene::Camera::Projection::Perspective);
    edited.createPrimitive(v3d::editor::document::Document::Primitive::Plane);
    const v3d::editor::scene::Items items = edited.scene()->items();
    BOOST_REQUIRE_EQUAL(items.size(), 2U);

    // nothing is selected, and the group still gathers the two items it was given
    BOOST_CHECK(edited.group(items).done);
    BOOST_CHECK_EQUAL(edited.scene()->count(), 3U);

    const v3d::editor::document::Outcome keyed = edited.key({ items.front() }, 12);
    BOOST_CHECK(keyed.done);
    BOOST_CHECK_EQUAL(keyed.message, "set a key at frame 12");

    BOOST_CHECK(!edited.settings(edited.scene()->renderSettings()).done);
    v3d::editor::scene::RenderSettings changed = edited.scene()->renderSettings();
    changed.samples += 1;
    BOOST_CHECK(edited.settings(changed).done);
    BOOST_CHECK(edited.scene()->renderSettings() == changed);
    edited.undo();
    BOOST_CHECK(!(edited.scene()->renderSettings() == changed));
}

/**
 * A save marks the document saved. Opening a project clears the history, so what came before it
 * cannot be undone, and the document opened counts as saved.
 **/
BOOST_AUTO_TEST_CASE(saving_and_opening_set_the_saved_position) {
    const Scratch scratch("saves");
    v3d::editor::document::Document edited = document(scratch.path());
    edited.createPrimitive(v3d::editor::document::Document::Primitive::Cone);
    BOOST_REQUIRE(edited.save());
    BOOST_CHECK(!edited.dirty());

    edited.createPrimitive(v3d::editor::document::Document::Primitive::Cube);
    BOOST_CHECK(edited.dirty());
    BOOST_REQUIRE(edited.open(edited.project()->directory() / v3d::editor::store::Project::MANIFEST));
    BOOST_CHECK(!edited.dirty());
    BOOST_CHECK_EQUAL(edited.scene()->count(), 1U);
    BOOST_CHECK(!edited.undo().done);
}

/**
 * An autosave runs once a minute while the document has changes, and none runs while it has
 * none. Recovering it leaves the document dirty, because the project does not hold it.
 **/
BOOST_AUTO_TEST_CASE(a_dirty_document_autosaves_on_the_interval) {
    const Scratch scratch("autosaves");
    v3d::editor::document::Document edited = document(scratch.path());

    edited.tick(v3d::editor::document::Document::autosaveInterval);
    BOOST_CHECK(!edited.recoverable());

    edited.createPrimitive(v3d::editor::document::Document::Primitive::Cylinder);
    edited.tick(v3d::editor::document::Document::autosaveInterval - 1);
    BOOST_CHECK(!edited.recoverable());
    edited.tick(1);
    BOOST_CHECK(edited.recoverable());

    v3d::editor::document::Document reopened = document(scratch.path());
    BOOST_REQUIRE(reopened.recover());
    BOOST_CHECK_EQUAL(reopened.scene()->count(), 1U);
    BOOST_CHECK(reopened.dirty());
}

/**
 * Going to a frame holds it within the timeline's range.
 **/
BOOST_AUTO_TEST_CASE(going_to_a_frame_holds_it_in_the_range) {
    const Scratch scratch("frames");
    v3d::editor::document::Document edited = document(scratch.path());
    edited.goTo(500);
    BOOST_CHECK_EQUAL(edited.scene()->timeline().frame, v3d::editor::scene::Timeline::DEFAULT_END);
    edited.goTo(-3);
    BOOST_CHECK_EQUAL(edited.scene()->timeline().frame, 1);
    // a frame is not a change to the document
    BOOST_CHECK(!edited.dirty());
}

/**
 * Auto key is turned on and off as a change to the document, which undo takes back. Asking for
 * it as it is changes nothing.
 **/
BOOST_AUTO_TEST_CASE(auto_key_is_a_change) {
    const Scratch scratch("auto_key");
    v3d::editor::document::Document edited = document(scratch.path());
    BOOST_CHECK(!edited.scene()->timeline().autoKey);

    const v3d::editor::document::Outcome on = edited.autoKey(true);
    BOOST_CHECK(on.done);
    BOOST_CHECK(on.undoable);
    BOOST_CHECK_EQUAL(on.message, "turned auto key on");
    BOOST_CHECK(edited.scene()->timeline().autoKey);
    BOOST_CHECK(edited.dirty());

    const v3d::editor::document::Outcome again = edited.autoKey(true);
    BOOST_CHECK(!again.done);
    BOOST_CHECK_EQUAL(edited.history()->undoDepth(), 1U);

    edited.undo();
    BOOST_CHECK(!edited.scene()->timeline().autoKey);
}

/**
 * The rate and range are set as one change, and one that cannot be a timeline is refused.
 **/
BOOST_AUTO_TEST_CASE(the_range_is_a_change) {
    const Scratch scratch("timing");
    v3d::editor::document::Document edited = document(scratch.path());

    const v3d::editor::document::Outcome set = edited.timing(30.0F, 10, 50);
    BOOST_CHECK(set.done);
    BOOST_CHECK(set.undoable);
    BOOST_CHECK_EQUAL(set.message, "set frames 10 to 50");
    BOOST_CHECK_EQUAL(edited.scene()->timeline().rate, 30.0F);
    BOOST_CHECK_EQUAL(edited.scene()->timeline().frame, 10);

    BOOST_CHECK(!edited.timing(30.0F, 10, 50).done);
    BOOST_CHECK(!edited.timing(30.0F, 50, 10).done);
    BOOST_CHECK(!edited.timing(0.0F, 1, 10).done);
    BOOST_CHECK_EQUAL(edited.history()->undoDepth(), 1U);

    edited.undo();
    BOOST_CHECK_EQUAL(edited.scene()->timeline().end, v3d::editor::scene::Timeline::DEFAULT_END);
}

/**
 * Keys are moved and deleted as one change each, and with nothing to act on nothing changes.
 **/
BOOST_AUTO_TEST_CASE(keys_are_moved_and_deleted) {
    const Scratch scratch("keys");
    v3d::editor::document::Document edited = document(scratch.path());
    edited.createPrimitive(v3d::editor::document::Document::Primitive::Cube);
    const unsigned int cube = edited.scene()->items()[0]->id();
    edited.key(edited.scene()->items(), 5);

    const v3d::editor::document::Outcome moved = edited.moveKeys({ { .item = cube, .frame = 5.0F } }, -2);
    BOOST_CHECK(moved.done);
    BOOST_CHECK_EQUAL(moved.message, "moved 1 key 2 frames earlier");
    BOOST_CHECK(edited.scene()->keyedFrames(edited.scene()->items()) == std::vector<float>({ 3.0F }));
    BOOST_CHECK(!edited.moveKeys({ { .item = cube, .frame = 3.0F } }, 0).done);
    BOOST_CHECK(!edited.moveKeys({ { .item = cube, .frame = 9.0F } }, 1).done);

    const v3d::editor::document::Outcome deleted = edited.deleteKeys({ { .item = cube, .frame = 3.0F } });
    BOOST_CHECK(deleted.done);
    BOOST_CHECK(deleted.undoable);
    BOOST_CHECK(edited.scene()->curves(cube).empty());
    BOOST_CHECK(!edited.deleteKeys({}).done);

    edited.undo();
    BOOST_CHECK(edited.scene()->keyedFrames(edited.scene()->items()) == std::vector<float>({ 3.0F }));
}

/**
 * A glTF file comes in as one change: its nodes become items with their hierarchy, its
 * triangles are joined back into quads, and its keys become curves. Undo takes every item out,
 * and redo brings back the items with their parents and curves.
 **/
BOOST_AUTO_TEST_CASE(a_gltf_file_imports_as_one_change) {
    const Scratch scratch("import");
    v3d::editor::document::Document edited = document(scratch.path());

    const v3d::editor::document::Outcome imported = edited.importScene(boost::filesystem::path(V3D_INTERCHANGE_DATA) / "scene.glb");
    BOOST_REQUIRE_MESSAGE(imported.done, imported.message);
    BOOST_CHECK_EQUAL(imported.message, "imported 4 items from scene.glb");
    BOOST_CHECK_EQUAL(edited.history()->undoDepth(), 1U);

    const auto check = [&edited]() {
        const boost::shared_ptr<v3d::editor::scene::Scene>& scene = edited.scene();
        BOOST_REQUIRE_EQUAL(scene->count(), 4U);
        boost::shared_ptr<v3d::editor::scene::Mesh> box;
        for (const boost::shared_ptr<v3d::editor::scene::Item>& item : scene->items()) {
            if (item->kind() == v3d::editor::scene::Item::Kind::Mesh) {
                box = v3d::editor::scene::as<v3d::editor::scene::Mesh>(item);
            }
        }
        BOOST_REQUIRE(box);
        BOOST_CHECK_EQUAL(box->geometry()->faceCount(), 6U);
        const boost::shared_ptr<v3d::editor::scene::Item> rig = scene->parent(box->id());
        BOOST_REQUIRE(rig);
        BOOST_CHECK(rig->kind() == v3d::editor::scene::Item::Kind::Group);
        BOOST_CHECK(rig->selected());
        BOOST_CHECK_EQUAL(scene->curves(box->id()).count("translation.x"), 1U);
    };
    check();

    BOOST_CHECK(edited.undo().done);
    BOOST_CHECK_EQUAL(edited.scene()->count(), 0U);
    BOOST_CHECK(edited.redo().done);
    check();
}

/**
 * A USD stage comes in the same way. Blender writes each object as a prim with its data
 * beneath it, so the box is a group holding a mesh of six quads. The stage's z up is turned to
 * y up at its top.
 **/
BOOST_AUTO_TEST_CASE(a_usd_stage_imports_as_one_change) {
    const Scratch scratch("usd");
    v3d::editor::document::Document edited = document(scratch.path());

    const v3d::editor::document::Outcome imported = edited.importScene(boost::filesystem::path(V3D_INTERCHANGE_DATA) / "scene.usda");
    BOOST_REQUIRE_MESSAGE(imported.done, imported.message);
    BOOST_CHECK_EQUAL(imported.message, "imported 8 items from scene.usda");

    const boost::shared_ptr<v3d::editor::scene::Scene>& scene = edited.scene();
    boost::shared_ptr<v3d::editor::scene::Mesh> box;
    for (const boost::shared_ptr<v3d::editor::scene::Item>& item : scene->items()) {
        if (item->kind() == v3d::editor::scene::Item::Kind::Mesh) {
            box = v3d::editor::scene::as<v3d::editor::scene::Mesh>(item);
        }
    }
    BOOST_REQUIRE(box);
    BOOST_CHECK_EQUAL(box->geometry()->faceCount(), 6U);
    const boost::shared_ptr<v3d::editor::scene::Item> holder = scene->parent(box->id());
    BOOST_REQUIRE(holder);
    BOOST_CHECK_EQUAL(scene->curves(holder->id()).count("translation.x"), 1U);
    BOOST_CHECK(edited.undo().done);
    BOOST_CHECK_EQUAL(scene->count(), 0U);
}

/**
 * A file that is neither glTF nor USD, or cannot be read, is refused and changes nothing.
 **/
BOOST_AUTO_TEST_CASE(an_import_that_cannot_be_read_changes_nothing) {
    const Scratch scratch("refused");
    v3d::editor::document::Document edited = document(scratch.path());

    const v3d::editor::document::Outcome wrong = edited.importScene(boost::filesystem::path(V3D_INTERCHANGE_DATA) / "make_scene_fixture.py");
    const v3d::editor::document::Outcome missing = edited.importScene(boost::filesystem::path(V3D_INTERCHANGE_DATA) / "missing.glb");

    BOOST_CHECK(!wrong.done);
    BOOST_CHECK_NE(wrong.message.find("neither glTF nor USD"), std::string::npos);
    BOOST_CHECK(!missing.done);
    BOOST_CHECK_NE(missing.message.find("missing.glb"), std::string::npos);
    BOOST_CHECK_EQUAL(edited.scene()->count(), 0U);
    BOOST_CHECK_EQUAL(edited.history()->undoDepth(), 0U);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
