/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Project.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <string>

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

v3d::editor::Document document(const boost::filesystem::path& project) {
    return v3d::editor::Document(boost::make_shared<v3d::log::Logger>(), project);
}

BOOST_AUTO_TEST_SUITE(document_test)

/**
 * An operation applies itself and records itself, so the change it made is one undo away, and
 * it says what it did. Undo and redo say what they stepped over.
 **/
BOOST_AUTO_TEST_CASE(an_operation_applies_and_records_itself) {
    const Scratch scratch("records");
    v3d::editor::Document edited = document(scratch.path());
    BOOST_CHECK(!edited.dirty());

    const v3d::editor::Outcome made = edited.createPrimitive(v3d::editor::Document::Primitive::Cube);
    BOOST_CHECK(made.done);
    BOOST_CHECK_EQUAL(made.message, "created a cube - 1 items");
    BOOST_CHECK_EQUAL(edited.scene()->count(), 1U);
    BOOST_CHECK_EQUAL(edited.history()->undoDepth(), 1U);
    BOOST_CHECK(edited.dirty());

    const v3d::editor::Outcome undone = edited.undo();
    BOOST_CHECK(undone.done);
    BOOST_CHECK_EQUAL(undone.message, "undo create cube");
    BOOST_CHECK_EQUAL(edited.scene()->count(), 0U);
    BOOST_CHECK(!edited.dirty());

    BOOST_CHECK(edited.redo().done);
    BOOST_CHECK_EQUAL(edited.scene()->count(), 1U);
    BOOST_CHECK(!edited.redo().done);
}

/**
 * An operation that cannot run changes nothing, records nothing, and says why.
 **/
BOOST_AUTO_TEST_CASE(an_operation_that_cannot_run_says_why) {
    const Scratch scratch("refuses");
    v3d::editor::Document edited = document(scratch.path());

    const v3d::editor::Outcome grouped = edited.group({});
    BOOST_CHECK(!grouped.done);
    BOOST_CHECK(!grouped.message.empty());

    const v3d::editor::Outcome split = edited.splitEdges();
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
    v3d::editor::Document edited = document(scratch.path());
    const v3d::type::camera::Profile at("Perspective");
    edited.createCamera(at, v3d::editor::Camera::Projection::Perspective);
    edited.createPrimitive(v3d::editor::Document::Primitive::Plane);
    const v3d::editor::Items items = edited.scene()->items();
    BOOST_REQUIRE_EQUAL(items.size(), 2U);

    // nothing is selected, and the group still gathers the two items it was given
    BOOST_CHECK(edited.group(items).done);
    BOOST_CHECK_EQUAL(edited.scene()->count(), 3U);

    const v3d::editor::Outcome keyed = edited.key({ items.front() }, 12);
    BOOST_CHECK(keyed.done);
    BOOST_CHECK_EQUAL(keyed.message, "set a key at frame 12");

    BOOST_CHECK(!edited.settings(edited.scene()->renderSettings()).done);
    v3d::editor::RenderSettings changed = edited.scene()->renderSettings();
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
    v3d::editor::Document edited = document(scratch.path());
    edited.createPrimitive(v3d::editor::Document::Primitive::Cone);
    BOOST_REQUIRE(edited.save());
    BOOST_CHECK(!edited.dirty());

    edited.createPrimitive(v3d::editor::Document::Primitive::Cube);
    BOOST_CHECK(edited.dirty());
    BOOST_REQUIRE(edited.open(edited.project()->directory() / v3d::editor::Project::MANIFEST));
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
    v3d::editor::Document edited = document(scratch.path());

    edited.tick(v3d::editor::Document::autosaveInterval);
    BOOST_CHECK(!edited.recoverable());

    edited.createPrimitive(v3d::editor::Document::Primitive::Cylinder);
    edited.tick(v3d::editor::Document::autosaveInterval - 1);
    BOOST_CHECK(!edited.recoverable());
    edited.tick(1);
    BOOST_CHECK(edited.recoverable());

    v3d::editor::Document reopened = document(scratch.path());
    BOOST_REQUIRE(reopened.recover());
    BOOST_CHECK_EQUAL(reopened.scene()->count(), 1U);
    BOOST_CHECK(reopened.dirty());
}

/**
 * Going to a frame holds it within the timeline's range.
 **/
BOOST_AUTO_TEST_CASE(going_to_a_frame_holds_it_in_the_range) {
    const Scratch scratch("frames");
    v3d::editor::Document edited = document(scratch.path());
    edited.goTo(500);
    BOOST_CHECK_EQUAL(edited.scene()->timeline().frame, v3d::editor::Timeline::DEFAULT_END);
    edited.goTo(-3);
    BOOST_CHECK_EQUAL(edited.scene()->timeline().frame, 1);
    // a frame is not a change to the document
    BOOST_CHECK(!edited.dirty());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
