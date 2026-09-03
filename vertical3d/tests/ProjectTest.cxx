/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <cstddef>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/test/unit_test.hpp>

#include "../src/CreatePoly.h"
#include "../src/Project.h"
#include "../src/Scene.h"

#include <boost/make_shared.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {

    boost::shared_ptr<v3d::log::Logger> logger() {
        return boost::make_shared<v3d::log::Logger>();
    }

    /**
     * A path to write a project to and read it back from.
     **/
    std::string scratch(const char* name) {
        return (boost::filesystem::temp_directory_path() / name).string();
    }

    void put(const std::string& path, const std::string& text) {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file.write(text.data(), static_cast<std::streamsize>(text.size()));
    }

    std::string get(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    /**
     * Collects a scene's meshes in the order it holds them.
     **/
    class Collector final : public v3d::editor::SceneVisitor {
     public:
        void visit(const boost::shared_ptr<v3d::brep::BRep>& mesh) override {
            meshes.push_back(mesh);
        }

        std::vector<boost::shared_ptr<v3d::brep::BRep>> meshes;
    };

    std::vector<boost::shared_ptr<v3d::brep::BRep>> meshes(const boost::shared_ptr<v3d::editor::Scene>& scene) {
        Collector collector;
        scene->accept(&collector);
        return collector.meshes;
    }

    /**
     * Whether two meshes hold the same geometry, the same topology and the same placement.
     * Ids are deliberately not compared - a mesh read from a file is a new node.
     **/
    void same(const boost::shared_ptr<v3d::brep::BRep>& left, const boost::shared_ptr<v3d::brep::BRep>& right) {
        BOOST_REQUIRE_EQUAL(left->vertexCount(), right->vertexCount());
        BOOST_REQUIRE_EQUAL(left->edgeCount(), right->edgeCount());
        BOOST_REQUIRE_EQUAL(left->faceCount(), right->faceCount());

        for (std::size_t index = 0; index < left->vertexCount(); index++) {
            const unsigned int id = static_cast<unsigned int>(index);
            BOOST_CHECK(left->vertex(id)->point() == right->vertex(id)->point());
        }
        for (std::size_t index = 0; index < left->edgeCount(); index++) {
            const unsigned int id = static_cast<unsigned int>(index);
            BOOST_CHECK_EQUAL(left->edge(id)->vertex(), right->edge(id)->vertex());
            BOOST_CHECK_EQUAL(left->edge(id)->face(), right->edge(id)->face());
            BOOST_CHECK_EQUAL(left->edge(id)->pair(), right->edge(id)->pair());
            BOOST_CHECK_EQUAL(left->edge(id)->next(), right->edge(id)->next());
        }
        for (std::size_t index = 0; index < left->faceCount(); index++) {
            const unsigned int id = static_cast<unsigned int>(index);
            BOOST_CHECK(left->face(id)->normal() == right->face(id)->normal());
            BOOST_CHECK_EQUAL(left->face(id)->edge(), right->face(id)->edge());
        }

        BOOST_CHECK(left->translation() == right->translation());
        BOOST_CHECK(left->scale() == right->scale());
        BOOST_CHECK(left->rotation() == right->rotation());
    }

};  // namespace

BOOST_AUTO_TEST_CASE(project_round_trip_test) {
    const std::string path = scratch("v3d_round_trip.json");

    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->translation(glm::vec3(1.5f, -2.0f, 0.25f));
    cube->scale(glm::vec3(2.0f, 0.5f, 1.0f));
    cube->rotation(glm::angleAxis(0.75f, glm::normalize(glm::vec3(0.0f, 1.0f, 1.0f))));

    boost::shared_ptr<v3d::editor::Scene> written = boost::make_shared<v3d::editor::Scene>();
    written->add(cube);

    v3d::editor::Project project(logger());
    project.name("round trip");
    BOOST_REQUIRE_EQUAL(project.write(path, written), true);

    boost::shared_ptr<v3d::editor::Scene> read = boost::make_shared<v3d::editor::Scene>();
    v3d::editor::Project reader(logger());
    BOOST_REQUIRE_EQUAL(reader.read(path, read), true);

    BOOST_CHECK_EQUAL(reader.name(), "round trip");
    BOOST_REQUIRE_EQUAL(read->count(), 1u);

    // the topology is stored as it stands, so a round trip renumbers nothing
    same(cube, meshes(read).front());

    boost::filesystem::remove(path);
}

BOOST_AUTO_TEST_CASE(project_order_and_selection_test) {
    const std::string path = scratch("v3d_order.json");

    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    boost::shared_ptr<v3d::brep::BRep> cone = v3d::editor::create_poly_cone();
    cube->selected(true);
    cone->face(0)->selected(true);

    boost::shared_ptr<v3d::editor::Scene> written = boost::make_shared<v3d::editor::Scene>();
    written->add(cube);
    written->add(cone);

    v3d::editor::Project project(logger());
    BOOST_REQUIRE_EQUAL(project.write(path, written), true);

    boost::shared_ptr<v3d::editor::Scene> read = boost::make_shared<v3d::editor::Scene>();
    BOOST_REQUIRE_EQUAL(v3d::editor::Project(logger()).read(path, read), true);
    BOOST_REQUIRE_EQUAL(read->count(), 2u);

    const std::vector<boost::shared_ptr<v3d::brep::BRep>> loaded = meshes(read);
    BOOST_REQUIRE_EQUAL(loaded.size(), 2u);
    same(cube, loaded[0]);
    same(cone, loaded[1]);

    // selection is where the user is rather than what the document holds, so nothing comes
    // back selected
    BOOST_CHECK_EQUAL(static_cast<bool>(read->selection()), false);
    BOOST_CHECK_EQUAL(loaded[1]->face(0)->selected(), false);

    boost::filesystem::remove(path);
}

BOOST_AUTO_TEST_CASE(project_read_replaces_the_scene_test) {
    const std::string path = scratch("v3d_empty.json");

    boost::shared_ptr<v3d::editor::Scene> written = boost::make_shared<v3d::editor::Scene>();
    v3d::editor::Project project(logger());
    BOOST_REQUIRE_EQUAL(project.write(path, written), true);

    boost::shared_ptr<v3d::editor::Scene> read = boost::make_shared<v3d::editor::Scene>();
    read->add(v3d::editor::create_poly_cube());
    BOOST_REQUIRE_EQUAL(v3d::editor::Project(logger()).read(path, read), true);

    // reading replaces the document rather than adding to it
    BOOST_CHECK_EQUAL(read->count(), 0u);

    boost::filesystem::remove(path);
}

BOOST_AUTO_TEST_CASE(project_written_form_test) {
    const std::string path = scratch("v3d_form.json");

    boost::shared_ptr<v3d::editor::Scene> written = boost::make_shared<v3d::editor::Scene>();
    written->add(v3d::editor::create_poly_plane());

    v3d::editor::Project project(logger());
    project.name("form");
    BOOST_REQUIRE_EQUAL(project.write(path, written), true);

    const std::string text = get(path);
    // indented rather than the one line boost::json serializes, so the file diffs
    BOOST_CHECK(text.find("\n") != std::string::npos);
    BOOST_CHECK(text.find("\"version\": 1") != std::string::npos);
    BOOST_CHECK(text.find("\"name\": \"form\"") != std::string::npos);
    // a vector stays on one line - a mesh broken a number to a line is unreadable
    BOOST_CHECK(text.find("\"normal\": [0, 1, 0]") != std::string::npos);

    boost::filesystem::remove(path);
}

BOOST_AUTO_TEST_CASE(project_missing_file_test) {
    boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    scene->add(v3d::editor::create_poly_cube());

    v3d::editor::Project project(logger());
    BOOST_CHECK_EQUAL(project.read(scratch("v3d_no_such_project.json"), scene), false);

    // a failed open does not also lose the document in memory
    BOOST_CHECK_EQUAL(scene->count(), 1u);
}

BOOST_AUTO_TEST_CASE(project_rejects_malformed_test) {
    const std::string path = scratch("v3d_malformed.json");

    boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    scene->add(v3d::editor::create_poly_cube());
    v3d::editor::Project project(logger());

    put(path, "{ this is not json");
    BOOST_CHECK_EQUAL(project.read(path, scene), false);

    // a document of another version is refused rather than read as far as it goes
    put(path, "{\"version\": 2, \"name\": \"later\", \"meshes\": []}");
    BOOST_CHECK_EQUAL(project.read(path, scene), false);

    put(path, "{\"name\": \"no version\", \"meshes\": []}");
    BOOST_CHECK_EQUAL(project.read(path, scene), false);

    put(path, "{\"version\": 1, \"name\": \"no meshes\"}");
    BOOST_CHECK_EQUAL(project.read(path, scene), false);

    // a mesh missing one of its three arrays
    put(path, "{\"version\": 1, \"meshes\": [{\"vertices\": [], \"edges\": []}]}");
    BOOST_CHECK_EQUAL(project.read(path, scene), false);

    // a vertex that is not three numbers
    put(path, "{\"version\": 1, \"meshes\": [{\"vertices\": [[0, 1]], \"edges\": [], \"faces\": []}]}");
    BOOST_CHECK_EQUAL(project.read(path, scene), false);

    BOOST_CHECK_EQUAL(scene->count(), 1u);
    boost::filesystem::remove(path);
}

BOOST_AUTO_TEST_CASE(project_rejects_dangling_reference_test) {
    const std::string path = scratch("v3d_dangling.json");

    boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    v3d::editor::Project project(logger());

    // one vertex and an edge naming a second that is not there. The wireframe and the picker
    // would walk off the end of the mesh, so the read refuses it
    put(path,
        "{\"version\": 1, \"meshes\": [{"
        "\"vertices\": [[0, 0, 0]],"
        "\"edges\": [{\"vertex\": 1, \"face\": 2147483648, \"pair\": 2147483648, \"next\": 0}],"
        "\"faces\": []}]}");
    BOOST_CHECK_EQUAL(project.read(path, scene), false);
    BOOST_CHECK_EQUAL(scene->count(), 0u);

    // a face naming an edge the mesh does not hold
    put(path,
        "{\"version\": 1, \"meshes\": [{"
        "\"vertices\": [[0, 0, 0]],"
        "\"edges\": [{\"vertex\": 0, \"face\": 0, \"pair\": 2147483648, \"next\": 0}],"
        "\"faces\": [{\"normal\": [0, 0, 1], \"edge\": 3}]}]}");
    BOOST_CHECK_EQUAL(project.read(path, scene), false);

    // the same mesh with the references it should have, which is what makes both of the
    // above a test of the check rather than of the parse
    put(path,
        "{\"version\": 1, \"meshes\": [{"
        "\"vertices\": [[0, 0, 0]],"
        "\"edges\": [{\"vertex\": 0, \"face\": 0, \"pair\": 2147483648, \"next\": 0}],"
        "\"faces\": [{\"normal\": [0, 0, 1], \"edge\": 0}]}]}");
    BOOST_CHECK_EQUAL(project.read(path, scene), true);
    BOOST_CHECK_EQUAL(scene->count(), 1u);

    // an unpaired edge is not a dangling one: INVALID_ID is what a boundary carries
    BOOST_CHECK_EQUAL(meshes(scene).front()->edge(0)->pair(), v3d::brep::INVALID_ID);

    boost::filesystem::remove(path);
}

BOOST_AUTO_TEST_CASE(project_default_transform_test) {
    const std::string path = scratch("v3d_no_transform.json");

    boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    v3d::editor::Project project(logger());

    // a hand written mesh with no placement sits at the origin, unturned and unscaled
    put(path, "{\"version\": 1, \"meshes\": [{\"vertices\": [], \"edges\": [], \"faces\": []}]}");
    BOOST_REQUIRE_EQUAL(project.read(path, scene), true);
    BOOST_REQUIRE_EQUAL(scene->count(), 1u);

    same(boost::make_shared<v3d::brep::BRep>(), meshes(scene).front());

    // and the name a file does not carry is the untitled one
    BOOST_CHECK_EQUAL(project.name(), "untitled");

    boost::filesystem::remove(path);
}
