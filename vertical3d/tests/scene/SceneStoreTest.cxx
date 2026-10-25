/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>
#include <api/brep/Vertex.h>
#include <api/database/Database.h>
#include <api/database/Statement.h>
#include <api/log/Logger.h>
#include <api/type/animation/Curve.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/CreatePoly.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/SceneStore.h>
#include <vertical3d/src/scene/Timeline.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {

boost::shared_ptr<v3d::log::Logger> logger() {
    return boost::make_shared<v3d::log::Logger>();
}

/**
 * A scene document in the temporary directory, removed before and after the test.
 **/
class Scratch final {
 public:
    explicit Scratch(const std::string& name) :
        path_(boost::filesystem::temp_directory_path() / ("v3d_store_" + name + ".v3d")) {
        boost::filesystem::remove(path_);
    }

    ~Scratch() {
        boost::system::error_code error;
        boost::filesystem::remove(path_, error);
    }

    Scratch(const Scratch&) = delete;
    Scratch& operator=(const Scratch&) = delete;

    std::string path() const {
        return path_.string();
    }

 private:
    boost::filesystem::path path_;
};

boost::shared_ptr<v3d::editor::Mesh> item(const boost::shared_ptr<v3d::brep::BRep>& geometry) {
    return boost::make_shared<v3d::editor::Mesh>(geometry);
}

/**
 * A scene of every kind, each placed and set away from its defaults: a cube, a camera, a light
 * and a cone, in that order.
 **/
boost::shared_ptr<v3d::editor::Scene> everyKind() {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();

    const boost::shared_ptr<v3d::editor::Mesh> cube = item(v3d::editor::create_poly_cube());
    cube->translation(glm::vec3(1.5F, -2.0F, 0.25F));
    cube->rotation(glm::angleAxis(0.7F, glm::normalize(glm::vec3(1.0F, 2.0F, 3.0F))));
    cube->scale(glm::vec3(2.0F, 0.5F, 3.0F));
    scene->add(cube);

    const boost::shared_ptr<v3d::editor::Camera> camera = boost::make_shared<v3d::editor::Camera>();
    camera->translation(glm::vec3(0.0F, 4.0F, -10.0F));
    camera->lens({ .projection = v3d::editor::Camera::Projection::Orthographic, .fov = 30.0F, .extent = 6.5F,
        .clipping = glm::vec2(0.5F, 250.0F) });
    scene->add(camera);

    const boost::shared_ptr<v3d::editor::Light> light = boost::make_shared<v3d::editor::Light>();
    light->rotation(glm::angleAxis(-0.3F, glm::vec3(1.0F, 0.0F, 0.0F)));
    light->emission({ .type = v3d::editor::Light::Type::Spot, .colour = glm::vec3(1.0F, 0.5F, 0.25F),
        .intensity = 40.0F, .cone = 0.4F, .delta = 0.1F });
    scene->add(light);

    scene->add(item(v3d::editor::create_poly_cone()));
    return scene;
}

/**
 * Whether two meshes hold the same parts, index for index.
 **/
void sameGeometry(const v3d::brep::BRep& left, const v3d::brep::BRep& right) {
    BOOST_REQUIRE_EQUAL(left.vertexCount(), right.vertexCount());
    BOOST_REQUIRE_EQUAL(left.edgeCount(), right.edgeCount());
    BOOST_REQUIRE_EQUAL(left.faceCount(), right.faceCount());
    for (v3d::brep::Index id = 0; id < left.vertexCount(); id++) {
        BOOST_CHECK(left.vertex(id)->point() == right.vertex(id)->point());
    }
    for (v3d::brep::Index id = 0; id < left.edgeCount(); id++) {
        BOOST_CHECK_EQUAL(left.edge(id)->vertex(), right.edge(id)->vertex());
        BOOST_CHECK_EQUAL(left.edge(id)->face(), right.edge(id)->face());
        BOOST_CHECK_EQUAL(left.edge(id)->pair(), right.edge(id)->pair());
        BOOST_CHECK_EQUAL(left.edge(id)->next(), right.edge(id)->next());
    }
    for (v3d::brep::Index id = 0; id < left.faceCount(); id++) {
        BOOST_CHECK(left.face(id)->normal() == right.face(id)->normal());
        BOOST_CHECK_EQUAL(left.face(id)->edge(), right.face(id)->edge());
    }
}

/**
 * Whether two items are the same document item: kind, document id, placement, and what the
 * kind holds, all exactly. Their run ids differ, because a read makes new items.
 **/
void same(const boost::shared_ptr<v3d::editor::Item>& left, const boost::shared_ptr<v3d::editor::Item>& right) {
    BOOST_REQUIRE(left->kind() == right->kind());
    BOOST_CHECK_EQUAL(left->documentId(), right->documentId());
    BOOST_CHECK(left->translation() == right->translation());
    BOOST_CHECK(left->rotation() == right->rotation());
    BOOST_CHECK(left->scale() == right->scale());
    switch (left->kind()) {
        case v3d::editor::Item::Kind::Mesh:
            sameGeometry(*v3d::editor::as<v3d::editor::Mesh>(left)->geometry(),
                *v3d::editor::as<v3d::editor::Mesh>(right)->geometry());
            break;
        case v3d::editor::Item::Kind::Camera: {
            const v3d::editor::Camera::Lens& expected = v3d::editor::as<v3d::editor::Camera>(left)->lens();
            const v3d::editor::Camera::Lens& actual = v3d::editor::as<v3d::editor::Camera>(right)->lens();
            BOOST_CHECK(expected.projection == actual.projection);
            BOOST_CHECK_EQUAL(expected.fov, actual.fov);
            BOOST_CHECK_EQUAL(expected.extent, actual.extent);
            BOOST_CHECK(expected.clipping == actual.clipping);
            break;
        }
        case v3d::editor::Item::Kind::Light: {
            const v3d::editor::Light::Emission& expected = v3d::editor::as<v3d::editor::Light>(left)->emission();
            const v3d::editor::Light::Emission& actual = v3d::editor::as<v3d::editor::Light>(right)->emission();
            BOOST_CHECK(expected.type == actual.type);
            BOOST_CHECK(expected.colour == actual.colour);
            BOOST_CHECK_EQUAL(expected.intensity, actual.intensity);
            BOOST_CHECK_EQUAL(expected.cone, actual.cone);
            BOOST_CHECK_EQUAL(expected.delta, actual.delta);
            break;
        }
        case v3d::editor::Item::Kind::Group:
            // a group holds nothing but its placement
            break;
    }
}

/**
 * Change a saved document directly, as a damaged file or a later build would.
 **/
void alter(const Scratch& scratch, const std::string& sql) {
    v3d::database::Database database;
    BOOST_REQUIRE(database.open(scratch.path(), v3d::database::Database::Mode::ReadWrite));
    BOOST_REQUIRE_MESSAGE(database.execute(sql), database.error());
}

/**
 * One integer a query on a saved document returns.
 **/
std::int64_t query(const Scratch& scratch, const std::string& sql) {
    v3d::database::Database database;
    BOOST_REQUIRE(database.open(scratch.path(), v3d::database::Database::Mode::ReadOnly));
    v3d::database::Statement statement = database.prepare(sql);
    BOOST_REQUIRE_MESSAGE(statement.valid(), database.error());
    BOOST_REQUIRE(statement.step() == v3d::database::Statement::Step::Row);
    return statement.integer(0);
}

BOOST_AUTO_TEST_CASE(store_round_trip_test) {
    const Scratch scratch("round_trip");
    const boost::shared_ptr<v3d::editor::Scene> written = everyKind();
    v3d::editor::SceneStore store(logger());
    BOOST_REQUIRE(store.write(scratch.path(), *written));

    v3d::editor::Scene read;
    BOOST_REQUIRE(store.read(scratch.path(), &read));

    // every kind comes back, in its order, with its document id and the next id to give
    BOOST_REQUIRE_EQUAL(read.count(), 4U);
    for (std::size_t index = 0; index < read.count(); index++) {
        same(written->items()[index], read.items()[index]);
    }
    BOOST_CHECK_EQUAL(read.nextDocumentId(), written->nextDocumentId());
}

BOOST_AUTO_TEST_CASE(store_selection_not_saved_test) {
    const Scratch scratch("selection");
    const boost::shared_ptr<v3d::editor::Scene> written = everyKind();
    for (const boost::shared_ptr<v3d::editor::Item>& each : written->items()) {
        each->selected(true);
    }
    v3d::editor::as<v3d::editor::Mesh>(written->items()[0])->geometry()->face(0)->selected(true);
    v3d::editor::SceneStore store(logger());
    BOOST_REQUIRE(store.write(scratch.path(), *written));

    // selection is where the user is rather than what the document holds
    v3d::editor::Scene read;
    BOOST_REQUIRE(store.read(scratch.path(), &read));
    BOOST_CHECK(read.selection().empty());
    BOOST_CHECK(!v3d::editor::as<v3d::editor::Mesh>(read.items()[0])->geometry()->face(0)->selected());
}

BOOST_AUTO_TEST_CASE(store_written_form_test) {
    const Scratch scratch("form");
    v3d::editor::SceneStore store(logger());
    BOOST_REQUIRE(store.write(scratch.path(), *everyKind()));

    // the header says what the database is, and which version of the tables it has
    BOOST_CHECK_EQUAL(query(scratch, "PRAGMA application_id"), v3d::editor::SceneStore::APPLICATION_ID);
    BOOST_CHECK_EQUAL(query(scratch, "PRAGMA user_version"), v3d::editor::SceneStore::SCHEMA_VERSION);

    // a row per item, each at its kind's version, and three blobs for each mesh, whose counts
    // match the mesh
    BOOST_CHECK_EQUAL(query(scratch, "SELECT count(*) FROM items"), 4);
    BOOST_CHECK_EQUAL(query(scratch, "SELECT count(*) FROM items WHERE kind = 'mesh' AND type_version = 1"), 2);
    BOOST_CHECK_EQUAL(query(scratch, "SELECT count(*) FROM buffers"), 6);
    BOOST_CHECK_EQUAL(query(scratch,
        "SELECT count FROM buffers JOIN items ON items.id = buffers.item WHERE position = 0 AND role = 'edges'"),
        static_cast<std::int64_t>(v3d::editor::create_poly_cube()->edgeCount()));
    BOOST_CHECK_EQUAL(query(scratch,
        "SELECT length(data) FROM buffers JOIN items ON items.id = buffers.item WHERE position = 0 AND role = 'vertices'"),
        static_cast<std::int64_t>(v3d::editor::create_poly_cube()->vertexCount() * 12));

    // a payload is JSON that SQLite can read inside
    BOOST_CHECK_EQUAL(query(scratch,
        "SELECT payload ->> '$.intensity' FROM items WHERE kind = 'light'"), 40);
}

BOOST_AUTO_TEST_CASE(store_rewrite_test) {
    const Scratch scratch("rewrite");
    v3d::editor::SceneStore store(logger());
    const boost::shared_ptr<v3d::editor::Scene> scene = everyKind();
    BOOST_REQUIRE(store.write(scratch.path(), *scene));

    // a second write replaces the whole of the first, blobs of a removed mesh included
    scene->remove(scene->items()[0]->id());
    BOOST_REQUIRE(store.write(scratch.path(), *scene));
    BOOST_CHECK_EQUAL(query(scratch, "SELECT count(*) FROM items"), 3);
    BOOST_CHECK_EQUAL(query(scratch, "SELECT count(*) FROM buffers"), 3);

    v3d::editor::Scene read;
    BOOST_REQUIRE(store.read(scratch.path(), &read));
    BOOST_CHECK(read.items()[0]->kind() == v3d::editor::Item::Kind::Camera);
}

BOOST_AUTO_TEST_CASE(store_document_ids_test) {
    const Scratch scratch("ids");
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Light> first = boost::make_shared<v3d::editor::Light>();
    const boost::shared_ptr<v3d::editor::Light> second = boost::make_shared<v3d::editor::Light>();
    scene.add(first);
    scene.add(second);
    scene.remove(second->id());

    v3d::editor::SceneStore store(logger());
    BOOST_REQUIRE(store.write(scratch.path(), scene));
    v3d::editor::Scene read;
    BOOST_REQUIRE(store.read(scratch.path(), &read));

    // the id the removed light had is not given again, in this run or after a read
    BOOST_CHECK_EQUAL(read.items()[0]->documentId(), first->documentId());
    const boost::shared_ptr<v3d::editor::Light> third = boost::make_shared<v3d::editor::Light>();
    read.add(third);
    BOOST_CHECK_EQUAL(third->documentId(), second->documentId() + 1);
}

BOOST_AUTO_TEST_CASE(store_failed_write_keeps_the_last_save_test) {
    const Scratch scratch("failed_write");
    v3d::editor::SceneStore store(logger());
    v3d::editor::Scene first;
    first.add(item(v3d::editor::create_poly_plane()));
    BOOST_REQUIRE(store.write(scratch.path(), first));

    // a write that fails part way, after it has already replaced the meshes, leaves nothing of
    // itself behind
    alter(scratch, "CREATE TRIGGER refuse AFTER INSERT ON items WHEN NEW.kind = 'light' "
        "BEGIN SELECT RAISE(ABORT, 'refused'); END;");
    const boost::shared_ptr<v3d::editor::Scene> second = everyKind();
    BOOST_CHECK(!store.write(scratch.path(), *second));

    v3d::editor::Scene read;
    BOOST_REQUIRE(store.read(scratch.path(), &read));
    BOOST_REQUIRE_EQUAL(read.count(), 1U);
    same(first.items()[0], read.items()[0]);
}

BOOST_AUTO_TEST_CASE(store_missing_document_test) {
    const Scratch scratch("missing");
    v3d::editor::SceneStore store(logger());
    v3d::editor::Scene scene;
    BOOST_CHECK(!store.read(scratch.path(), &scene));

    // a read never makes a document
    BOOST_CHECK(!boost::filesystem::exists(scratch.path()));
    BOOST_CHECK(!store.read(scratch.path(), nullptr));
}

BOOST_AUTO_TEST_CASE(store_refuses_another_programs_database_test) {
    const Scratch scratch("foreign");
    {
        v3d::database::Database database;
        BOOST_REQUIRE(database.open(scratch.path(), v3d::database::Database::Mode::Create));
        BOOST_REQUIRE(database.execute("CREATE TABLE theirs (value INTEGER); INSERT INTO theirs VALUES (7);"));
    }
    v3d::editor::SceneStore store(logger());
    v3d::editor::Scene scene;
    BOOST_CHECK(!store.read(scratch.path(), &scene));

    // and a write does not overwrite it
    BOOST_CHECK(!store.write(scratch.path(), *everyKind()));
    BOOST_CHECK_EQUAL(query(scratch, "SELECT value FROM theirs"), 7);
}

/**
 * A document damaged one way, and what the damage is.
 **/
struct Damage {
    const char* what;
    const char* sql;
};

BOOST_AUTO_TEST_CASE(store_refuses_a_damaged_document_test) {
    const std::vector<Damage> damages = {
        { "a light out of range", "UPDATE items SET payload = json_set(payload, '$.intensity', -1) WHERE kind = 'light'" },
        { "a light missing a field", "UPDATE items SET payload = json_remove(payload, '$.delta') WHERE kind = 'light'" },
        { "a light with a field its version does not have",
            "UPDATE items SET payload = json_set(payload, '$.falloff', 2) WHERE kind = 'light'" },
        { "a light of an unknown type", "UPDATE items SET payload = json_set(payload, '$.type', 'area') WHERE kind = 'light'" },
        { "a camera with a lens out of range",
            "UPDATE items SET payload = json_set(payload, '$.clipping', json('[5, 1]')) WHERE kind = 'camera'" },
        { "a camera from a later build", "UPDATE items SET type_version = 2 WHERE kind = 'camera'" },
        { "a mesh at a version no build wrote", "UPDATE items SET type_version = 0 WHERE kind = 'mesh'" },
        { "a mesh with a payload", "UPDATE items SET payload = '{\"smooth\": true}' WHERE kind = 'mesh'" },
        { "a mesh missing a blob", "DELETE FROM buffers WHERE role = 'faces' AND item = (SELECT min(id) FROM items)" },
        { "a mesh whose blob is short", "UPDATE buffers SET count = count + 1 WHERE role = 'vertices'" },
        { "an unknown kind", "UPDATE items SET kind = 'curve' WHERE kind = 'camera'" },
        { "a blob that belongs to no mesh",
            "INSERT INTO buffers (item, role, count, data) SELECT id, 'extra', 0, X'' FROM items WHERE kind = 'camera'" },
        { "a next id that is not above every id", "UPDATE meta SET value = 1 WHERE key = 'next_id'" },
        { "tables from a later build", "PRAGMA user_version = 4" },
        { "a curve on a property its owner does not have",
            "INSERT INTO curves SELECT id, 'lens.fov', 1, 0, X'' FROM items WHERE kind = 'light'" },
        { "a curve with no keys", "INSERT INTO curves SELECT id, 'translation.x', 1, 0, X'' FROM items WHERE kind = 'light'" },
        { "a curve from a later build",
            "INSERT INTO curves SELECT id, 'translation.x', 2, 0, X'' FROM items WHERE kind = 'light'" },
        { "a curve whose keys are short",
            "INSERT INTO curves SELECT id, 'translation.x', 1, 1, X'00' FROM items WHERE kind = 'light'" },
        { "a curve whose keys do not rise",
            "INSERT INTO curves SELECT id, 'translation.x', 1, 2, "
            "X'00000040000000000000000000000000020100000000803F00000000000000000000000002010000' "
            "FROM items WHERE kind = 'light'" },
        { "a key of an unknown interpolation",
            "INSERT INTO curves SELECT id, 'translation.x', 1, 1, X'0000803F00000000000000000000000009010000' "
            "FROM items WHERE kind = 'light'" },
        { "a timeline with no frame rate", "INSERT OR REPLACE INTO meta VALUES ('rate', 0.0)" },
        { "a frame outside the range", "INSERT OR REPLACE INTO meta VALUES ('frame', 500)" },
        { "tables at a version no build wrote", "PRAGMA user_version = 0" },
        { "another program's database", "PRAGMA application_id = 1" },
        { "an item that is its own parent", "UPDATE items SET parent = id WHERE kind = 'camera'" },
        { "a parent the document does not hold",
            "PRAGMA foreign_keys = OFF; UPDATE items SET parent = 99 WHERE kind = 'light'" },
        { "a group with a payload",
            "UPDATE items SET kind = 'group', payload = '{\"open\": true}' WHERE kind = 'camera'" },
    };

    v3d::editor::SceneStore store(logger());
    for (const Damage& damage : damages) {
        BOOST_TEST_CONTEXT(damage.what) {
            const Scratch scratch("damaged");
            BOOST_REQUIRE(store.write(scratch.path(), *everyKind()));
            alter(scratch, damage.sql);

            // the read fails whole, and the scene in memory is left as it was
            v3d::editor::Scene scene;
            const boost::shared_ptr<v3d::editor::Mesh> kept = item(v3d::editor::create_poly_plane());
            scene.add(kept);
            BOOST_CHECK(!store.read(scratch.path(), &scene));
            BOOST_REQUIRE_EQUAL(scene.count(), 1U);
            BOOST_CHECK(scene.items()[0] == kept);
        }
    }
}

BOOST_AUTO_TEST_CASE(store_refuses_a_dangling_reference_test) {
    const Scratch scratch("dangling");
    const boost::shared_ptr<v3d::brep::BRep> broken = v3d::editor::create_poly_plane();
    v3d::brep::HalfEdge edge;
    edge.vertex(99);
    edge.face(0);
    edge.pair(v3d::brep::INVALID_ID);
    edge.next(0);
    broken->addEdge(edge);

    // the write keeps what it is given, and the read refuses what the wireframe would index past
    v3d::editor::Scene written;
    written.add(item(broken));
    v3d::editor::SceneStore store(logger());
    BOOST_REQUIRE(store.write(scratch.path(), written));
    v3d::editor::Scene read;
    BOOST_CHECK(!store.read(scratch.path(), &read));
    BOOST_CHECK_EQUAL(read.count(), 0U);
}

BOOST_AUTO_TEST_CASE(store_writes_only_the_changes_test) {
    const Scratch scratch("changes");
    v3d::editor::SceneStore store(logger());
    const boost::shared_ptr<v3d::editor::Scene> scene = everyKind();
    BOOST_REQUIRE(store.write(scratch.path(), *scene));
    BOOST_CHECK(store.written().whole);
    BOOST_CHECK_EQUAL(store.written().rows, 4U);

    // a save that changes nothing writes nothing
    BOOST_REQUIRE(store.write(scratch.path(), *scene));
    BOOST_CHECK(!store.written().whole);
    BOOST_CHECK_EQUAL(store.written().rows, 0U);
    BOOST_CHECK_EQUAL(store.written().moved, 0U);
    BOOST_CHECK_EQUAL(store.written().removed, 0U);

    // a move writes the one row, and the change reads back
    scene->items()[1]->translation(glm::vec3(7.0F, 0.0F, 0.0F));
    BOOST_REQUIRE(store.write(scratch.path(), *scene));
    BOOST_CHECK_EQUAL(store.written().rows, 1U);
    v3d::editor::Scene read;
    v3d::editor::SceneStore reader(logger());
    BOOST_REQUIRE(reader.read(scratch.path(), &read));
    BOOST_CHECK(read.items()[1]->translation() == glm::vec3(7.0F, 0.0F, 0.0F));

    // so does a change to a mesh's geometry made in place, once it is counted
    const boost::shared_ptr<v3d::editor::Mesh> cube = v3d::editor::as<v3d::editor::Mesh>(scene->items()[0]);
    cube->geometry()->vertex(0)->point(glm::vec3(3.0F));
    cube->geometry()->touch();
    BOOST_REQUIRE(store.write(scratch.path(), *scene));
    BOOST_CHECK_EQUAL(store.written().rows, 1U);
    BOOST_REQUIRE(reader.read(scratch.path(), &read));
    BOOST_CHECK(v3d::editor::as<v3d::editor::Mesh>(read.items()[0])->geometry()->vertex(0)->point() == glm::vec3(3.0F));
}

BOOST_AUTO_TEST_CASE(store_removes_and_moves_rows_test) {
    const Scratch scratch("removes");
    v3d::editor::SceneStore store(logger());
    const boost::shared_ptr<v3d::editor::Scene> scene = everyKind();
    BOOST_REQUIRE(store.write(scratch.path(), *scene));

    // removing the first item deletes its row and its blobs, and the rest only change place
    const boost::shared_ptr<v3d::editor::Item> cube = scene->items()[0];
    scene->remove(cube->id());
    BOOST_REQUIRE(store.write(scratch.path(), *scene));
    BOOST_CHECK_EQUAL(store.written().removed, 1U);
    BOOST_CHECK_EQUAL(store.written().moved, 3U);
    BOOST_CHECK_EQUAL(store.written().rows, 0U);
    BOOST_CHECK_EQUAL(query(scratch, "SELECT count(*) FROM items"), 3);
    BOOST_CHECK_EQUAL(query(scratch, "SELECT count(*) FROM buffers"), 3);

    // putting it back, as a redone creation does, writes it again with its id
    scene->add(cube);
    BOOST_REQUIRE(store.write(scratch.path(), *scene));
    BOOST_CHECK_EQUAL(store.written().rows, 1U);
    v3d::editor::Scene read;
    v3d::editor::SceneStore reader(logger());
    BOOST_REQUIRE(reader.read(scratch.path(), &read));
    BOOST_REQUIRE_EQUAL(read.count(), 4U);
    same(cube, read.items()[3]);
    BOOST_CHECK(read.items()[0]->kind() == v3d::editor::Item::Kind::Camera);
}

BOOST_AUTO_TEST_CASE(store_reads_then_writes_only_the_changes_test) {
    const Scratch scratch("read_then_write");
    {
        v3d::editor::SceneStore first(logger());
        BOOST_REQUIRE(first.write(scratch.path(), *everyKind()));
    }

    // a store that read a document remembers it, so its next save writes only what changed
    v3d::editor::SceneStore store(logger());
    v3d::editor::Scene scene;
    BOOST_REQUIRE(store.read(scratch.path(), &scene));
    v3d::editor::as<v3d::editor::Light>(scene.items()[2])->emission({ .intensity = 3.0F });
    BOOST_REQUIRE(store.write(scratch.path(), scene));
    BOOST_CHECK(!store.written().whole);
    BOOST_CHECK_EQUAL(store.written().rows, 1U);
}

BOOST_AUTO_TEST_CASE(store_writes_another_document_whole_test) {
    const Scratch first("whole_first");
    const Scratch second("whole_second");
    v3d::editor::SceneStore store(logger());
    const boost::shared_ptr<v3d::editor::Scene> scene = everyKind();
    BOOST_REQUIRE(store.write(first.path(), *scene));

    // a document the store has not written or read is written whole, as Save As does, and
    // becomes the one it remembers
    BOOST_REQUIRE(store.write(second.path(), *scene));
    BOOST_CHECK(store.written().whole);
    BOOST_CHECK_EQUAL(store.written().rows, 4U);
    BOOST_REQUIRE(store.write(second.path(), *scene));
    BOOST_CHECK(!store.written().whole);

    // and one another store wrote is written whole, rather than merged into
    v3d::editor::SceneStore other(logger());
    v3d::editor::Scene small;
    small.add(item(v3d::editor::create_poly_plane()));
    BOOST_REQUIRE(other.write(first.path(), small));
    BOOST_REQUIRE(store.write(second.path(), *scene));
    BOOST_REQUIRE(other.write(first.path(), small));
    BOOST_CHECK(!other.written().whole);
    BOOST_REQUIRE(store.write(first.path(), *scene));
    BOOST_CHECK(store.written().whole);
    BOOST_CHECK_EQUAL(query(first, "SELECT count(*) FROM items"), 4);
}

BOOST_AUTO_TEST_CASE(store_failed_write_is_followed_by_a_whole_one_test) {
    const Scratch scratch("after_failure");
    v3d::editor::SceneStore store(logger());
    const boost::shared_ptr<v3d::editor::Scene> scene = everyKind();
    BOOST_REQUIRE(store.write(scratch.path(), *scene));

    // a write that fails leaves the document as the last save left it, and the store forgets
    // what it remembered of it
    alter(scratch, "CREATE TRIGGER refuse BEFORE UPDATE ON items BEGIN SELECT RAISE(ABORT, 'refused'); END;");
    scene->remove(scene->items()[0]->id());
    BOOST_CHECK(!store.write(scratch.path(), *scene));
    BOOST_CHECK_EQUAL(query(scratch, "SELECT count(*) FROM items"), 4);

    alter(scratch, "DROP TRIGGER refuse");
    BOOST_REQUIRE(store.write(scratch.path(), *scene));
    BOOST_CHECK(store.written().whole);
    BOOST_CHECK_EQUAL(query(scratch, "SELECT count(*) FROM items"), 3);
}

BOOST_AUTO_TEST_CASE(store_hierarchy_round_trip_test) {
    const Scratch scratch("hierarchy");
    const boost::shared_ptr<v3d::editor::Scene> written = everyKind();
    const v3d::editor::Items kinds = written->items();
    const boost::shared_ptr<v3d::editor::Group> group = boost::make_shared<v3d::editor::Group>();
    group->translation(glm::vec3(0.0F, 3.0F, 0.0F));
    group->rotation(glm::angleAxis(0.5F, glm::vec3(0.0F, 1.0F, 0.0F)));
    written->add(group);
    // the cube beneath the group, and the light beneath the cube, both added before it
    BOOST_REQUIRE(written->parent(kinds[0]->id(), group->id()));
    BOOST_REQUIRE(written->parent(kinds[2]->id(), kinds[0]->id()));

    v3d::editor::SceneStore store(logger());
    BOOST_REQUIRE(store.write(scratch.path(), *written));
    BOOST_CHECK_EQUAL(query(scratch, "SELECT count(*) FROM items WHERE parent IS NULL"), 3);
    BOOST_CHECK_EQUAL(query(scratch, "SELECT count(*) FROM items WHERE kind = 'group' AND type_version = 1"), 1);

    v3d::editor::Scene read;
    BOOST_REQUIRE(store.read(scratch.path(), &read));

    // every item comes back with its parent, and so where it was in the world
    BOOST_REQUIRE_EQUAL(read.count(), 5U);
    const v3d::editor::Evaluation before = written->evaluate();
    const v3d::editor::Evaluation after = read.evaluate();
    for (std::size_t index = 0; index < read.count(); index++) {
        same(written->items()[index], read.items()[index]);
        const boost::shared_ptr<v3d::editor::Item> parent = written->parent(written->items()[index]->id());
        const boost::shared_ptr<v3d::editor::Item> back = read.parent(read.items()[index]->id());
        BOOST_REQUIRE_EQUAL(static_cast<bool>(parent), static_cast<bool>(back));
        if (parent) {
            BOOST_CHECK_EQUAL(parent->documentId(), back->documentId());
        }
        BOOST_CHECK(before.placed(written->items()[index]->id()).world == after.placed(read.items()[index]->id()).world);
    }
}

BOOST_AUTO_TEST_CASE(store_parent_change_moves_a_row_test) {
    const Scratch scratch("reparent");
    const boost::shared_ptr<v3d::editor::Scene> scene = everyKind();
    const boost::shared_ptr<v3d::editor::Item> cube = scene->items()[0];
    const boost::shared_ptr<v3d::editor::Item> light = scene->items()[2];
    scene->parent(light->id(), cube->id());
    v3d::editor::SceneStore store(logger());
    BOOST_REQUIRE(store.write(scratch.path(), *scene));

    // a parent is the scene's rather than the item's, so a change to it alone moves the row
    // and writes nothing else
    scene->parent(light->id(), 0);
    BOOST_REQUIRE(store.write(scratch.path(), *scene));
    BOOST_CHECK_EQUAL(store.written().rows, 0U);
    BOOST_CHECK_EQUAL(store.written().moved, 1U);
    BOOST_CHECK_EQUAL(query(scratch, "SELECT count(*) FROM items WHERE parent IS NULL"), 4);

    // a changed parent is written beside its children, which still name it, in the same
    // write
    scene->parent(light->id(), cube->id());
    cube->translate(glm::vec3(1.0F, 0.0F, 0.0F));
    BOOST_REQUIRE(store.write(scratch.path(), *scene));
    BOOST_CHECK_EQUAL(store.written().rows, 1U);
    BOOST_CHECK_EQUAL(store.written().moved, 1U);
    v3d::editor::Scene read;
    BOOST_REQUIRE(store.read(scratch.path(), &read));
    BOOST_CHECK(read.parent(read.items()[2]->id()) == read.items()[0]);
}

BOOST_AUTO_TEST_CASE(store_animation_round_trip_test) {
    const Scratch scratch("animation");
    const boost::shared_ptr<v3d::editor::Scene> written = everyKind();
    const v3d::editor::Items kinds = written->items();
    const std::vector<v3d::type::animation::Curve::Key> keys{
        { .frame = 1.0F, .value = 0.0F, .interpolation = v3d::type::animation::Curve::Interpolation::Step },
        { .frame = 8.5F, .value = 2.0F, .in = 0.5F, .out = -1.0F, .automatic = false },
        { .frame = 20.0F, .value = 5.0F } };
    const v3d::type::animation::Curve shaped(keys);
    v3d::type::animation::Curve fade;
    fade.set(2.0F, 40.0F);
    fade.set(12.0F, 0.0F);
    BOOST_REQUIRE(written->curve(kinds[0]->id(), "translation.y", shaped));
    BOOST_REQUIRE(written->curve(kinds[2]->id(), "emission.intensity", fade));
    written->timeline({ .rate = 30.0F, .start = 2, .end = 48, .frame = 7 });

    v3d::editor::SceneStore store(logger());
    BOOST_REQUIRE(store.write(scratch.path(), *written));
    BOOST_CHECK_EQUAL(store.written().curves, 2U);
    BOOST_CHECK_EQUAL(query(scratch, "SELECT count(*) FROM curves"), 2);

    // every curve comes back key for key, with the timeline, and the scene posed at its frame
    v3d::editor::Scene read;
    BOOST_REQUIRE(store.read(scratch.path(), &read));
    BOOST_CHECK(read.timeline() == written->timeline());
    BOOST_REQUIRE_EQUAL(read.curves(read.items()[0]->id()).size(), 1U);
    BOOST_CHECK(read.curves(read.items()[0]->id()).at("translation.y") == shaped);
    BOOST_CHECK(read.curves(read.items()[2]->id()).at("emission.intensity") == fade);
    BOOST_CHECK_EQUAL(read.items()[0]->translation().y, 0.0F);
    BOOST_CHECK_CLOSE(v3d::editor::as<v3d::editor::Light>(read.items()[2])->emission().intensity, fade.sample(7.0F), 0.001F);

    // the read is what the store remembers, pose and all, so writing it back writes nothing
    BOOST_REQUIRE(store.write(scratch.path(), read));
    BOOST_CHECK_EQUAL(store.written().rows, 0U);
    BOOST_CHECK_EQUAL(store.written().curves, 0U);
}

BOOST_AUTO_TEST_CASE(store_changed_curve_alone_test) {
    const Scratch scratch("curve_change");
    const boost::shared_ptr<v3d::editor::Scene> scene = everyKind();
    const v3d::editor::Items kinds = scene->items();
    v3d::type::animation::Curve curve;
    curve.set(1.0F, 0.0F);
    BOOST_REQUIRE(scene->curve(kinds[0]->id(), "scale.x", curve));
    BOOST_REQUIRE(scene->curve(kinds[1]->id(), "lens.fov", curve));
    v3d::editor::SceneStore store(logger());
    BOOST_REQUIRE(store.write(scratch.path(), *scene));

    // a curve belongs to the scene rather than its item, so a changed key writes the curve's
    // row and no item's
    curve.set(9.0F, 1.0F);
    BOOST_REQUIRE(scene->curve(kinds[0]->id(), "scale.x", curve));
    BOOST_REQUIRE(store.write(scratch.path(), *scene));
    BOOST_CHECK_EQUAL(store.written().rows, 0U);
    BOOST_CHECK_EQUAL(store.written().curves, 1U);
    BOOST_CHECK_EQUAL(query(scratch, "SELECT count FROM curves WHERE property = 'scale.x'"), 2);

    // and a curve taken away deletes its row, as does removing the item that had one
    BOOST_REQUIRE(scene->curve(kinds[0]->id(), "scale.x", v3d::type::animation::Curve()));
    scene->remove(kinds[1]->id());
    BOOST_REQUIRE(store.write(scratch.path(), *scene));
    BOOST_CHECK_EQUAL(store.written().curves, 2U);
    BOOST_CHECK_EQUAL(query(scratch, "SELECT count(*) FROM curves"), 0);
}

BOOST_AUTO_TEST_CASE(store_reads_and_migrates_version_1_test) {
    const Scratch scratch("version_1");
    {
        // the tables as version 1 made them, which had no parent column
        v3d::database::Database database;
        BOOST_REQUIRE(database.open(scratch.path(), v3d::database::Database::Mode::Create));
        BOOST_REQUIRE_MESSAGE(database.execute(
            "CREATE TABLE meta (key TEXT PRIMARY KEY NOT NULL, value ANY NOT NULL) STRICT;"
            "CREATE TABLE items (id INTEGER PRIMARY KEY, position INTEGER NOT NULL, kind TEXT NOT NULL,"
            "  type_version INTEGER NOT NULL, tx REAL NOT NULL, ty REAL NOT NULL, tz REAL NOT NULL,"
            "  rx REAL NOT NULL, ry REAL NOT NULL, rz REAL NOT NULL, rw REAL NOT NULL,"
            "  sx REAL NOT NULL, sy REAL NOT NULL, sz REAL NOT NULL,"
            "  payload TEXT NOT NULL CHECK (json_valid(payload))) STRICT;"
            "CREATE TABLE buffers (item INTEGER NOT NULL REFERENCES items(id) ON DELETE CASCADE,"
            "  role TEXT NOT NULL, count INTEGER NOT NULL CHECK (count >= 0), data BLOB NOT NULL,"
            "  PRIMARY KEY (item, role)) STRICT;"
            "INSERT INTO items VALUES (1, 0, 'light', 1, 2, 0, 0, 0, 0, 0, 1, 1, 1, 1,"
            "  '{\"type\": \"point\", \"colour\": [1, 1, 1], \"intensity\": 1, \"cone\": 0.5236, \"delta\": 0.0873}');"
            "INSERT INTO meta VALUES ('next_id', 2);"
            "PRAGMA user_version = 1;"), database.error());
        BOOST_REQUIRE(database.applicationId(v3d::editor::SceneStore::APPLICATION_ID));
    }

    // a read takes the document as it is, with every item at the top of the scene
    v3d::editor::SceneStore store(logger());
    v3d::editor::Scene scene;
    BOOST_REQUIRE(store.read(scratch.path(), &scene));
    BOOST_REQUIRE_EQUAL(scene.count(), 1U);
    BOOST_CHECK(!scene.parent(scene.items()[0]->id()));
    BOOST_CHECK_EQUAL(query(scratch, "PRAGMA user_version"), 1);

    // the first write to it brings its tables up to date, and can then save a parent and a
    // curve
    const boost::shared_ptr<v3d::editor::Group> group = boost::make_shared<v3d::editor::Group>();
    scene.add(group);
    scene.parent(scene.items()[0]->id(), group->id());
    v3d::type::animation::Curve curve;
    curve.set(3.0F, 1.0F);
    BOOST_REQUIRE(scene.curve(group->id(), "rotation.y", curve));
    BOOST_REQUIRE(store.write(scratch.path(), scene));
    BOOST_CHECK(!store.written().whole);
    BOOST_CHECK_EQUAL(query(scratch, "PRAGMA user_version"), v3d::editor::SceneStore::SCHEMA_VERSION);
    v3d::editor::Scene read;
    BOOST_REQUIRE(store.read(scratch.path(), &read));
    BOOST_REQUIRE_EQUAL(read.count(), 2U);
    BOOST_CHECK(read.parent(read.items()[0]->id()) == read.items()[1]);
    BOOST_CHECK(read.curves(read.items()[1]->id()).at("rotation.y") == curve);
}

};  // namespace
