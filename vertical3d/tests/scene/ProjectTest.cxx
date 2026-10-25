/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <vertical3d/src/scene/CreatePoly.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Project.h>
#include <vertical3d/src/scene/Scene.h>

#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec3.hpp>

namespace {

boost::shared_ptr<v3d::log::Logger> logger() {
    return boost::make_shared<v3d::log::Logger>();
}

/**
 * A project directory in the temporary directory, removed before and after the test.
 **/
class Scratch final {
 public:
    explicit Scratch(const std::string& name) :
        path_(boost::filesystem::temp_directory_path() / ("v3d_project_" + name)) {
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

/**
 * A scene of a cube and a light.
 **/
boost::shared_ptr<v3d::editor::Scene> sample() {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    scene->add(boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube()));
    scene->add(boost::make_shared<v3d::editor::Light>());
    return scene;
}

std::string get(const boost::filesystem::path& path) {
    std::ifstream file(path.string(), std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void put(const boost::filesystem::path& path, const std::string& text) {
    std::ofstream file(path.string(), std::ios::binary | std::ios::trunc);
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
}

BOOST_AUTO_TEST_CASE(project_save_makes_the_layout_test) {
    const Scratch scratch("layout");
    v3d::editor::Project project(logger(), scratch.path());
    project.name("orrery");
    BOOST_REQUIRE(project.save(sample()));

    // every directory of a project, the scene in its place, and the manifest at the root
    for (const std::string& part : v3d::editor::Project::layout()) {
        BOOST_CHECK(boost::filesystem::is_directory(scratch.path() / part));
    }
    BOOST_CHECK(boost::filesystem::is_regular_file(scratch.path() / "scenes" / "main.v3d"));
    BOOST_CHECK(project.scene() == scratch.path() / "scenes" / "main.v3d");

    const boost::json::value manifest = boost::json::parse(get(scratch.path() / v3d::editor::Project::MANIFEST));
    BOOST_CHECK_EQUAL(manifest.at("version").as_int64(), v3d::editor::Project::VERSION);
    BOOST_CHECK_EQUAL(manifest.at("format").as_string(), "v3d-project");
    BOOST_CHECK_EQUAL(manifest.at("name").as_string(), "orrery");
    BOOST_REQUIRE_EQUAL(manifest.at("scenes").as_array().size(), 1U);
    BOOST_CHECK_EQUAL(manifest.at("scenes").as_array()[0].as_string(), "scenes/main.v3d");
}

BOOST_AUTO_TEST_CASE(project_open_test) {
    const Scratch scratch("open");
    v3d::editor::Project saved(logger(), scratch.path());
    saved.name("orrery");
    BOOST_REQUIRE(saved.save(sample()));

    // a project is opened by its manifest, and takes its directory and its name from it
    const Scratch elsewhere("elsewhere");
    v3d::editor::Project opened(logger(), elsewhere.path());
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    BOOST_REQUIRE(opened.open(scratch.path() / v3d::editor::Project::MANIFEST, scene));
    BOOST_CHECK(opened.directory() == scratch.path());
    BOOST_CHECK_EQUAL(opened.name(), "orrery");
    BOOST_CHECK_EQUAL(scene->count(), 2U);
}

BOOST_AUTO_TEST_CASE(project_save_as_test) {
    const Scratch first("first");
    const Scratch second("second");
    v3d::editor::Project project(logger(), first.path());
    BOOST_REQUIRE(project.save(sample()));

    // a new project becomes the one a save writes to
    BOOST_REQUIRE(project.saveAs(second.path(), sample()));
    BOOST_CHECK(project.directory() == second.path());
    BOOST_CHECK(boost::filesystem::exists(second.path() / v3d::editor::Project::MANIFEST));

    // and one cannot go where something already is
    BOOST_CHECK(!project.saveAs(first.path(), sample()));
    BOOST_CHECK(project.directory() == second.path());
}

BOOST_AUTO_TEST_CASE(project_refuses_a_bad_manifest_test) {
    const std::vector<std::string> manifests = {
        "not json",
        "{\"version\": 1, \"format\": \"something-else\", \"name\": \"x\", \"scenes\": [\"scenes/main.v3d\"]}",
        "{\"version\": 2, \"format\": \"v3d-project\", \"name\": \"x\", \"scenes\": [\"scenes/main.v3d\"]}",
        "{\"version\": 1, \"format\": \"v3d-project\", \"scenes\": [\"scenes/main.v3d\"]}",
        "{\"version\": 1, \"format\": \"v3d-project\", \"name\": \"x\", \"scenes\": []}",
        "{\"version\": 1, \"format\": \"v3d-project\", \"name\": \"x\", \"scenes\": [\"../main.v3d\"]}",
        "{\"version\": 1, \"format\": \"v3d-project\", \"name\": \"x\", \"scenes\": [\"C:/main.v3d\"]}",
        "{\"version\": 1, \"format\": \"v3d-project\", \"name\": \"x\", \"scenes\": [\"scenes/missing.v3d\"]}",
    };
    const Scratch scratch("bad");
    v3d::editor::Project saved(logger(), scratch.path());
    BOOST_REQUIRE(saved.save(sample()));
    const Scratch elsewhere("unchanged");

    for (const std::string& text : manifests) {
        BOOST_TEST_CONTEXT(text) {
            put(scratch.path() / v3d::editor::Project::MANIFEST, text);

            // nothing changes, the scene and the project included
            v3d::editor::Project project(logger(), elsewhere.path());
            const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
            scene->add(boost::make_shared<v3d::editor::Light>());
            BOOST_CHECK(!project.open(scratch.path() / v3d::editor::Project::MANIFEST, scene));
            BOOST_CHECK(project.directory() == elsewhere.path());
            BOOST_CHECK_EQUAL(project.name(), "untitled");
            BOOST_CHECK_EQUAL(scene->count(), 1U);
        }
    }

    v3d::editor::Project project(logger(), elsewhere.path());
    BOOST_CHECK(!project.open(scratch.path() / "no_such_manifest.json", boost::make_shared<v3d::editor::Scene>()));
}

BOOST_AUTO_TEST_CASE(project_autosave_test) {
    const Scratch scratch("autosave");
    v3d::editor::Project project(logger(), scratch.path());
    const boost::shared_ptr<v3d::editor::Scene> scene = sample();
    BOOST_REQUIRE(project.save(scene));
    BOOST_CHECK(!project.recoverable());

    // an autosave writes the change into the recovery database and leaves the document alone
    scene->items()[1]->translation(glm::vec3(9.0F, 0.0F, 0.0F));
    BOOST_REQUIRE(project.autosave(scene));
    BOOST_CHECK(boost::filesystem::is_regular_file(project.recovery()));
    BOOST_CHECK(project.recovery() == scratch.path() / "autosave" / "main.v3d");
    v3d::editor::Project document(logger(), scratch.path());
    const boost::shared_ptr<v3d::editor::Scene> saved = boost::make_shared<v3d::editor::Scene>();
    BOOST_REQUIRE(document.open(scratch.path() / v3d::editor::Project::MANIFEST, saved));
    BOOST_CHECK(saved->items()[1]->translation() == glm::vec3(0.0F));

    // a second autosave writes only what changed since the first
    scene->items()[0]->translation(glm::vec3(0.0F, 2.0F, 0.0F));
    BOOST_REQUIRE(project.autosave(scene));
    BOOST_CHECK(!project.autosaved().whole);
    BOOST_CHECK_EQUAL(project.autosaved().rows, 1U);

    // the recovery database is newer than the document, so it is offered, and holds both changes
    boost::filesystem::last_write_time(project.recovery(), boost::filesystem::last_write_time(project.scene()) + 10);
    BOOST_CHECK(project.recoverable());
    const boost::shared_ptr<v3d::editor::Scene> recovered = boost::make_shared<v3d::editor::Scene>();
    BOOST_REQUIRE(document.recover(recovered));
    BOOST_CHECK(recovered->items()[1]->translation() == glm::vec3(9.0F, 0.0F, 0.0F));
    BOOST_CHECK(recovered->items()[0]->translation() == glm::vec3(0.0F, 2.0F, 0.0F));

    // an older one holds nothing a save did not keep
    boost::filesystem::last_write_time(project.recovery(), boost::filesystem::last_write_time(project.scene()) - 10);
    BOOST_CHECK(!project.recoverable());

    // a save removes it
    BOOST_REQUIRE(project.save(scene));
    BOOST_CHECK(!boost::filesystem::exists(project.recovery()));
    BOOST_CHECK(!project.recoverable());
}

BOOST_AUTO_TEST_CASE(project_recovery_without_a_document_test) {
    const Scratch scratch("unsaved");
    v3d::editor::Project project(logger(), scratch.path());

    // a project never saved has only its autosave, which is offered
    BOOST_CHECK(!project.recoverable());
    BOOST_REQUIRE(project.autosave(sample()));
    BOOST_CHECK(!boost::filesystem::exists(project.scene()));
    BOOST_CHECK(project.recoverable());

    // and discarding it removes it
    project.discardRecovery();
    BOOST_CHECK(!project.recoverable());
    BOOST_CHECK(!project.recover(boost::make_shared<v3d::editor::Scene>()));
}

};  // namespace
