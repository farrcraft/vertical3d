/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SceneStore.h"

#include <api/brep/BRep.h>
#include <api/brep/Face.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>
#include <api/brep/Vertex.h>
#include <api/database/Database.h>
#include <api/database/Statement.h>
#include <api/database/Transaction.h>
#include <api/log/Logger.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Camera.h"
#include "Item.h"
#include "Light.h"
#include "Mesh.h"
#include "Scene.h"

#include <boost/filesystem.hpp>
#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

namespace {

// blobs are written as the machine holds them, which is the order the format defines
static_assert(std::endian::native == std::endian::little, "scene blobs are little-endian");

/**
 * The tables of a scene document at SCHEMA_VERSION.
 *
 * An item's kind is not checked by the table, so a new kind needs no new schema. Its payload
 * must be JSON. A buffer belongs to one item and goes when the item does.
 **/
const char* const SCHEMA =
"CREATE TABLE meta ("
"  key TEXT PRIMARY KEY NOT NULL,"
"  value ANY NOT NULL) STRICT;"
"CREATE TABLE items ("
"  id INTEGER PRIMARY KEY,"
"  position INTEGER NOT NULL,"
"  kind TEXT NOT NULL,"
"  type_version INTEGER NOT NULL,"
"  tx REAL NOT NULL, ty REAL NOT NULL, tz REAL NOT NULL,"
"  rx REAL NOT NULL, ry REAL NOT NULL, rz REAL NOT NULL, rw REAL NOT NULL,"
"  sx REAL NOT NULL, sy REAL NOT NULL, sz REAL NOT NULL,"
"  payload TEXT NOT NULL CHECK (json_valid(payload))) STRICT;"
"CREATE TABLE buffers ("
"  item INTEGER NOT NULL REFERENCES items(id) ON DELETE CASCADE,"
"  role TEXT NOT NULL,"
"  count INTEGER NOT NULL CHECK (count >= 0),"
"  data BLOB NOT NULL,"
"  PRIMARY KEY (item, role)) STRICT;";

/**
 * The size of one entry of each of a mesh's three blobs. A vertex is its point. A half edge is
 * its vertex, face, pair and next. A face is its normal and its first half edge.
 **/
constexpr std::size_t VERTEX_SIZE = 3 * sizeof(float);
constexpr std::size_t EDGE_SIZE = 4 * sizeof(std::uint32_t);
constexpr std::size_t FACE_SIZE = (3 * sizeof(float)) + sizeof(std::uint32_t);

/**
 * A perspective camera's angle of view is below a straight angle, in degrees.
 **/
constexpr float STRAIGHT_ANGLE = 180.0F;

/**
 * Append a number to a blob.
 **/
template <typename T>
void put(std::vector<std::byte>* bytes, T value) {
    const std::array<std::byte, sizeof(T)> raw = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
    bytes->insert(bytes->end(), raw.begin(), raw.end());
}

void put(std::vector<std::byte>* bytes, const glm::vec3& value) {
    put(bytes, value.x);
    put(bytes, value.y);
    put(bytes, value.z);
}

/**
 * Reads a blob's numbers in the order they were written. The caller has checked that the blob
 * holds every number it reads.
 **/
class Cursor final {
 public:
    explicit Cursor(std::span<const std::byte> bytes) :
        bytes_(bytes) {
    }

    template <typename T>
    T next() {
        std::array<std::byte, sizeof(T)> raw{};
        std::memcpy(raw.data(), bytes_.subspan(offset_, sizeof(T)).data(), sizeof(T));
        offset_ += sizeof(T);
        return std::bit_cast<T>(raw);
    }

    glm::vec3 vector() {
        const float x = next<float>();
        const float y = next<float>();
        const float z = next<float>();
        return glm::vec3(x, y, z);
    }

 private:
    std::span<const std::byte> bytes_;
    std::size_t offset_ = 0;
};

/**
 * Reads the columns of a row in order, from the first.
 **/
class Columns final {
 public:
    explicit Columns(const v3d::database::Statement& row) :
        row_(row) {
    }

    std::int64_t integer() {
        return row_.integer(column_++);
    }

    float real() {
        return static_cast<float>(row_.real(column_++));
    }

    std::string text() {
        return row_.text(column_++);
    }

    std::span<const std::byte> blob() {
        return row_.blob(column_++);
    }

    glm::vec3 vector() {
        const float x = real();
        const float y = real();
        const float z = real();
        return glm::vec3(x, y, z);
    }

 private:
    const v3d::database::Statement& row_;
    int column_ = 0;
};

/**
 * Binds a statement's parameters in order, from the first, after resetting it. It remembers
 * whether every bind succeeded.
 **/
class Binder final {
 public:
    explicit Binder(v3d::database::Statement* statement) :
        statement_(statement),
        bound_(statement->reset()) {
    }

    Binder& integer(std::int64_t value) {
        bound_ = bound_ && statement_->bindInteger(column_++, value);
        return *this;
    }

    Binder& real(float value) {
        bound_ = bound_ && statement_->bindReal(column_++, value);
        return *this;
    }

    Binder& text(const std::string& value) {
        bound_ = bound_ && statement_->bindText(column_++, value);
        return *this;
    }

    Binder& blob(std::span<const std::byte> value) {
        bound_ = bound_ && statement_->bindBlob(column_++, value);
        return *this;
    }

    Binder& vector(const glm::vec3& value) {
        return real(value.x).real(value.y).real(value.z);
    }

    /**
     * @return whether every bind succeeded and the statement then ran to its end
     **/
    bool run() {
        return bound_ && statement_->step() == v3d::database::Statement::Step::Done;
    }

 private:
    v3d::database::Statement* statement_;
    int column_ = 1;
    bool bound_;
};

/**
 * A mesh's three blobs, by role.
 **/
struct Topology {
    std::vector<std::byte> vertices;
    std::vector<std::byte> edges;
    std::vector<std::byte> faces;
};

Topology encode(const v3d::brep::BRep& mesh) {
    Topology topology;
    for (v3d::brep::Index id = 0; id < mesh.vertexCount(); id++) {
        put(&topology.vertices, mesh.vertex(id)->point());
    }
    for (v3d::brep::Index id = 0; id < mesh.edgeCount(); id++) {
        const v3d::brep::HalfEdge* edge = mesh.edge(id);
        put(&topology.edges, edge->vertex());
        put(&topology.edges, edge->face());
        put(&topology.edges, edge->pair());
        put(&topology.edges, edge->next());
    }
    for (v3d::brep::Index id = 0; id < mesh.faceCount(); id++) {
        const v3d::brep::Face* face = mesh.face(id);
        put(&topology.faces, face->normal());
        put(&topology.faces, face->edge());
    }
    return topology;
}

/**
 * One blob as it was read: how many entries its row says it holds, and its bytes.
 **/
struct Blob {
    std::int64_t count = 0;
    std::vector<std::byte> data;
};

/**
 * @return whether a blob holds exactly count entries of a size, and no more than an index can
 *         number
 **/
bool sized(const Blob& blob, std::size_t size, const char* role, std::string* problem) {
    if (blob.count < 0 || blob.count >= static_cast<std::int64_t>(v3d::brep::INVALID_ID) ||
        static_cast<std::size_t>(blob.count) * size != blob.data.size()) {
        *problem = std::string("its ") + role + " do not fill their blob exactly";
        return false;
    }
    return true;
}

bool finite(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

/**
 * Build a mesh from its three blobs, index for index.
 **/
bool decode(const std::map<std::string, Blob>& blobs, v3d::brep::BRep* mesh, std::string* problem) {
    if (blobs.size() != 3 || !blobs.contains("vertices") || !blobs.contains("edges") || !blobs.contains("faces")) {
        *problem = "it does not have exactly its vertices, edges and faces";
        return false;
    }
    const Blob& vertices = blobs.at("vertices");
    const Blob& edges = blobs.at("edges");
    const Blob& faces = blobs.at("faces");
    if (!sized(vertices, VERTEX_SIZE, "vertices", problem) || !sized(edges, EDGE_SIZE, "edges", problem) ||
        !sized(faces, FACE_SIZE, "faces", problem)) {
        return false;
    }

    Cursor points(vertices.data);
    for (std::int64_t entry = 0; entry < vertices.count; entry++) {
        const glm::vec3 point = points.vector();
        if (!finite(point)) {
            *problem = "a vertex is not a finite point";
            return false;
        }
        mesh->addVertex(v3d::brep::Vertex(point));
    }
    Cursor halves(edges.data);
    for (std::int64_t entry = 0; entry < edges.count; entry++) {
        v3d::brep::HalfEdge edge;
        edge.vertex(halves.next<std::uint32_t>());
        edge.face(halves.next<std::uint32_t>());
        edge.pair(halves.next<std::uint32_t>());
        edge.next(halves.next<std::uint32_t>());
        mesh->addEdge(edge);
    }
    Cursor loops(faces.data);
    for (std::int64_t entry = 0; entry < faces.count; entry++) {
        const glm::vec3 normal = loops.vector();
        const std::uint32_t first = loops.next<std::uint32_t>();
        if (!finite(normal)) {
            *problem = "a face's normal is not finite";
            return false;
        }
        mesh->addFace(v3d::brep::Face(normal, first));
    }

    // a reference out of range would make the wireframe and the picker index past the end of
    // the mesh, so it is refused here rather than found by whatever reads it first
    return mesh->validate(problem);
}

/**
 * @return whether a payload has exactly these keys
 **/
bool exactly(const boost::json::object& payload, std::initializer_list<const char*> keys, std::string* problem) {
    for (const char* key : keys) {
        if (!payload.contains(key)) {
            *problem = std::string("its payload has no ") + key;
            return false;
        }
    }
    if (payload.size() != keys.size()) {
        *problem = "its payload has a field its version does not define";
        return false;
    }
    return true;
}

/**
 * Read a finite number, or count of them, from a payload whose keys have been checked.
 **/
bool numbers(const boost::json::value& value, std::size_t count, float* out) {
    if (count == 1) {
        if (!value.is_number()) {
            return false;
        }
        out[0] = static_cast<float>(value.to_number<double>());
        return std::isfinite(out[0]);
    }
    if (!value.is_array() || value.as_array().size() != count) {
        return false;
    }
    for (std::size_t index = 0; index < count; index++) {
        if (!numbers(value.as_array()[index], 1, &out[index])) {
            return false;
        }
    }
    return true;
}

/**
 * @return a payload's string, or nothing when the value is not one
 **/
std::string_view word(const boost::json::value& value) {
    return value.is_string() ? std::string_view(value.as_string()) : std::string_view();
}

boost::json::array vector(const glm::vec3& value) {
    return boost::json::array{ value.x, value.y, value.z };
}

boost::json::object cameraPayload(const Camera& camera) {
    const Camera::Lens& lens = camera.lens();
    boost::json::object payload;
    payload["projection"] = lens.projection == Camera::Projection::Orthographic ? "orthographic" : "perspective";
    payload["fov"] = lens.fov;
    payload["extent"] = lens.extent;
    payload["clipping"] = boost::json::array{ lens.clipping.x, lens.clipping.y };
    return payload;
}

bool readCamera(const boost::json::object& payload, Camera* camera, std::string* problem) {
    if (!exactly(payload, { "projection", "fov", "extent", "clipping" }, problem)) {
        return false;
    }
    Camera::Lens lens;
    const std::string_view projection = word(payload.at("projection"));
    if (projection == "perspective") {
        lens.projection = Camera::Projection::Perspective;
    } else if (projection == "orthographic") {
        lens.projection = Camera::Projection::Orthographic;
    } else {
        *problem = "its projection is neither perspective nor orthographic";
        return false;
    }
    float clipping[2] = { 0.0F, 0.0F };
    if (!numbers(payload.at("fov"), 1, &lens.fov) || !numbers(payload.at("extent"), 1, &lens.extent) ||
        !numbers(payload.at("clipping"), 2, clipping)) {
        *problem = "a field of its lens is not a finite number";
        return false;
    }
    lens.clipping = glm::vec2(clipping[0], clipping[1]);
    if (lens.fov <= 0.0F || lens.fov >= STRAIGHT_ANGLE || lens.extent <= 0.0F || lens.clipping.x <= 0.0F ||
        lens.clipping.y <= lens.clipping.x) {
        *problem = "its lens is out of range";
        return false;
    }
    camera->lens(lens);
    return true;
}

boost::json::object lightPayload(const Light& light) {
    const Light::Emission& emission = light.emission();
    boost::json::object payload;
    switch (emission.type) {
        case Light::Type::Point:
            payload["type"] = "point";
            break;
        case Light::Type::Distant:
            payload["type"] = "distant";
            break;
        case Light::Type::Spot:
            payload["type"] = "spot";
            break;
    }
    payload["colour"] = vector(emission.colour);
    payload["intensity"] = emission.intensity;
    payload["cone"] = emission.cone;
    payload["delta"] = emission.delta;
    return payload;
}

bool readLight(const boost::json::object& payload, Light* light, std::string* problem) {
    if (!exactly(payload, { "type", "colour", "intensity", "cone", "delta" }, problem)) {
        return false;
    }
    Light::Emission emission;
    const std::string_view type = word(payload.at("type"));
    if (type == "point") {
        emission.type = Light::Type::Point;
    } else if (type == "distant") {
        emission.type = Light::Type::Distant;
    } else if (type == "spot") {
        emission.type = Light::Type::Spot;
    } else {
        *problem = "its type is not point, distant or spot";
        return false;
    }
    float colour[3] = { 0.0F, 0.0F, 0.0F };
    if (!numbers(payload.at("colour"), 3, colour) || !numbers(payload.at("intensity"), 1, &emission.intensity) ||
        !numbers(payload.at("cone"), 1, &emission.cone) || !numbers(payload.at("delta"), 1, &emission.delta)) {
        *problem = "a field of its emission is not a finite number";
        return false;
    }
    emission.colour = glm::vec3(colour[0], colour[1], colour[2]);
    if (emission.colour.r < 0.0F || emission.colour.g < 0.0F || emission.colour.b < 0.0F ||
        emission.intensity < 0.0F || emission.cone <= 0.0F || emission.cone >= glm::pi<float>() ||
        emission.delta < 0.0F || emission.delta > emission.cone) {
        *problem = "its emission is out of range";
        return false;
    }
    light->emission(emission);
    return true;
}

/**
 * Whether a row's kind is at the version this build writes.
 *
 * An older version is taken forward here one step at a time. Every kind is at its first
 * version, so a version below it is not one any build wrote, and is refused.
 **/
bool current(const std::string& kind, std::int64_t version, std::int64_t latest, std::string* problem) {
    if (version > latest) {
        *problem = "its " + kind + " was written by a later build";
        return false;
    }
    if (version < latest) {
        *problem = "its " + kind + " has no version this build can read";
        return false;
    }
    return true;
}

/**
 * Check a document's header against this build.
 *
 * @param fresh set when the database is empty and has no header at all, so it can be made
 *        into a document. Null when that is not allowed
 **/
bool header(v3d::database::Database* database, bool* fresh, std::string* problem) {
    std::int32_t application = 0;
    std::int32_t version = 0;
    if (!database->applicationId(&application) || !database->userVersion(&version)) {
        *problem = database->error();
        return false;
    }
    if (application == 0 && version == 0 && fresh != nullptr) {
        v3d::database::Statement tables = database->prepare("SELECT count(*) FROM sqlite_schema");
        if (tables.step() == v3d::database::Statement::Step::Row && tables.integer(0) == 0) {
            *fresh = true;
            return true;
        }
    }
    if (application != SceneStore::APPLICATION_ID) {
        *problem = "it is not a scene document";
        return false;
    }
    if (version > SceneStore::SCHEMA_VERSION) {
        *problem = "it was written by a later build";
        return false;
    }
    // an older schema is migrated forward here one version at a time. This is the first
    // version, so a lower one is refused
    if (version < SceneStore::SCHEMA_VERSION) {
        *problem = "it has no schema version this build can read";
        return false;
    }
    return true;
}

/**
 * The geometry of one mesh item, from its blobs.
 **/
boost::shared_ptr<Item> readMesh(std::int64_t id, const boost::json::object& payload, v3d::database::Statement* buffers,
    std::string* problem) {
    if (!exactly(payload, {}, problem)) {
        return nullptr;
    }
    if (!buffers->reset() || !buffers->bindInteger(1, id)) {
        *problem = buffers->error();
        return nullptr;
    }
    std::map<std::string, Blob> blobs;
    while (buffers->step() == v3d::database::Statement::Step::Row) {
        Columns row(*buffers);
        const std::string role = row.text();
        const std::int64_t count = row.integer();
        const std::span<const std::byte> data = row.blob();
        blobs[role] = Blob{ .count = count, .data = std::vector<std::byte>(data.begin(), data.end()) };
    }
    const boost::shared_ptr<v3d::brep::BRep> geometry = boost::make_shared<v3d::brep::BRep>();
    if (!decode(blobs, geometry.get(), problem)) {
        return nullptr;
    }
    return boost::make_shared<Mesh>(geometry);
}

/**
 * The item one row of the items table describes, without its id or its placement.
 **/
boost::shared_ptr<Item> readKind(std::int64_t id, const std::string& kind, std::int64_t version,
    const boost::json::object& payload, v3d::database::Statement* buffers, std::string* problem) {
    if (kind == "mesh") {
        return current(kind, version, SceneStore::MESH_VERSION, problem) ? readMesh(id, payload, buffers, problem) : nullptr;
    }
    if (kind == "camera") {
        const boost::shared_ptr<Camera> camera = boost::make_shared<Camera>();
        const bool read = current(kind, version, SceneStore::CAMERA_VERSION, problem) && readCamera(payload, camera.get(), problem);
        return read ? camera : nullptr;
    }
    if (kind == "light") {
        const boost::shared_ptr<Light> light = boost::make_shared<Light>();
        const bool read = current(kind, version, SceneStore::LIGHT_VERSION, problem) && readLight(payload, light.get(), problem);
        return read ? light : nullptr;
    }
    std::string unknown = "its kind, ";
    unknown += kind;
    unknown += ", is not one this build reads";
    *problem = unknown;
    return nullptr;
}

/**
 * The item a row of the items table describes, whole.
 **/
boost::shared_ptr<Item> readItem(const v3d::database::Statement& rows, v3d::database::Statement* buffers,
    std::string* problem) {
    Columns row(rows);
    const std::int64_t id = row.integer();
    const std::string kind = row.text();
    const std::int64_t version = row.integer();
    const glm::vec3 translation = row.vector();
    const glm::vec3 axis = row.vector();
    const float w = row.real();
    const glm::vec3 scale = row.vector();
    const std::string text = row.text();

    if (id <= 0 || !finite(translation) || !finite(axis) || !std::isfinite(w) || !finite(scale)) {
        *problem = "its id or its placement is not valid";
        return nullptr;
    }
    boost::system::error_code error;
    const boost::json::value parsed = boost::json::parse(text, error);
    if (error || !parsed.is_object()) {
        *problem = "its payload is not an object";
        return nullptr;
    }
    boost::shared_ptr<Item> item = readKind(id, kind, version, parsed.as_object(), buffers, problem);
    if (!item) {
        return nullptr;
    }
    item->documentId(id);
    item->translation(translation);
    item->rotation(glm::quat(w, axis.x, axis.y, axis.z));
    item->scale(scale);
    return item;
}

/**
 * Every item one document holds, read before any of it reaches the scene.
 **/
struct Loaded {
    Items items;
    std::int64_t next = 1;
};

bool load(v3d::database::Database* database, Loaded* loaded, std::string* problem) {
    v3d::database::Statement rows = database->prepare(
        "SELECT id, kind, type_version, tx, ty, tz, rx, ry, rz, rw, sx, sy, sz, payload "
        "FROM items ORDER BY position, id");
    v3d::database::Statement buffers = database->prepare("SELECT role, count, data FROM buffers WHERE item = ?1");
    if (!rows.valid() || !buffers.valid()) {
        *problem = database->error();
        return false;
    }

    std::int64_t highest = 0;
    std::size_t meshes = 0;
    v3d::database::Statement::Step step = rows.step();
    for (; step == v3d::database::Statement::Step::Row; step = rows.step()) {
        std::string fault;
        const boost::shared_ptr<Item> item = readItem(rows, &buffers, &fault);
        if (!item) {
            *problem = "item " + std::to_string(rows.integer(0)) + ": " + fault;
            return false;
        }
        loaded->items.push_back(item);
        highest = std::max(highest, item->documentId());
        meshes += item->kind() == Item::Kind::Mesh ? 1 : 0;
    }
    if (step == v3d::database::Statement::Step::Failed) {
        *problem = rows.error();
        return false;
    }

    // a blob that belongs to no mesh is data this build does not read
    v3d::database::Statement count = database->prepare("SELECT count(*) FROM buffers");
    if (count.step() != v3d::database::Statement::Step::Row ||
        static_cast<std::size_t>(count.integer(0)) != meshes * 3) {
        *problem = "it holds blobs that belong to no mesh";
        return false;
    }

    v3d::database::Statement next = database->prepare("SELECT value FROM meta WHERE key = 'next_id'");
    if (next.step() != v3d::database::Statement::Step::Row || next.integer(0) <= highest) {
        *problem = "its next id is missing or is not above every id it holds";
        return false;
    }
    loaded->next = next.integer(0);
    return true;
}

/**
 * The two inserts a write runs for each item.
 **/
struct Inserts {
    v3d::database::Statement item;
    v3d::database::Statement buffer;
};

/**
 * Write one item's row, and a mesh's blobs.
 **/
bool storeItem(Inserts* inserts, const boost::shared_ptr<Item>& item, std::int64_t position) {
    std::string kind;
    std::int64_t version = 0;
    std::string payload = "{}";
    switch (item->kind()) {
        case Item::Kind::Mesh:
            kind = "mesh";
            version = SceneStore::MESH_VERSION;
            break;
        case Item::Kind::Camera:
            kind = "camera";
            version = SceneStore::CAMERA_VERSION;
            payload = boost::json::serialize(cameraPayload(*boost::static_pointer_cast<Camera>(item)));
            break;
        case Item::Kind::Light:
            kind = "light";
            version = SceneStore::LIGHT_VERSION;
            payload = boost::json::serialize(lightPayload(*boost::static_pointer_cast<Light>(item)));
            break;
    }

    const glm::quat rotation = item->rotation();
    if (!Binder(&inserts->item).integer(item->documentId()).integer(position).text(kind).integer(version)
        .vector(item->translation()).vector(glm::vec3(rotation.x, rotation.y, rotation.z)).real(rotation.w)
        .vector(item->scale()).text(payload).run()) {
        return false;
    }

    const boost::shared_ptr<Mesh> mesh = as<Mesh>(item);
    if (!mesh) {
        return true;
    }
    const v3d::brep::BRep& geometry = *mesh->geometry();
    const Topology topology = encode(geometry);
    const auto blob = [&](const char* role, std::size_t count, const std::vector<std::byte>& data) {
        return Binder(&inserts->buffer).integer(item->documentId()).text(role).integer(static_cast<std::int64_t>(count))
            .blob(data).run();
    };
    return blob("vertices", geometry.vertexCount(), topology.vertices) &&
        blob("edges", geometry.edgeCount(), topology.edges) &&
        blob("faces", geometry.faceCount(), topology.faces);
}

/**
 * Write every item and the next id. The tables are empty when this starts.
 **/
bool store(v3d::database::Database* database, const Scene& scene, std::string* problem) {
    Inserts inserts{
        .item = database->prepare(
            "INSERT INTO items (id, position, kind, type_version, tx, ty, tz, rx, ry, rz, rw, sx, sy, sz, payload) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"),
        .buffer = database->prepare("INSERT INTO buffers (item, role, count, data) VALUES (?, ?, ?, ?)") };
    v3d::database::Statement next = database->prepare("INSERT INTO meta (key, value) VALUES ('next_id', ?)");
    if (!inserts.item.valid() || !inserts.buffer.valid() || !next.valid()) {
        *problem = database->error();
        return false;
    }

    std::int64_t position = 0;
    for (const boost::shared_ptr<Item>& item : scene.items()) {
        if (!storeItem(&inserts, item, position)) {
            *problem = !inserts.item.error().empty() ? inserts.item.error() : inserts.buffer.error();
            return false;
        }
        position++;
    }
    if (!Binder(&next).integer(scene.nextDocumentId()).run()) {
        *problem = next.error();
        return false;
    }
    return true;
}

};  // namespace

/**
 **/
SceneStore::SceneStore(const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger) {
}

/**
 **/
bool SceneStore::read(const std::string& path, Scene* scene) const {
    if (scene == nullptr) {
        return false;
    }
    v3d::database::Database database;
    if (!database.open(path, v3d::database::Database::Mode::ReadOnly)) {
        logger_->get()->error("No scene to read at {}: {}", path, database.error());
        return false;
    }

    std::string problem;
    Loaded loaded;
    if (!header(&database, nullptr, &problem) || !load(&database, &loaded, &problem)) {
        logger_->get()->error("Cannot read the scene {}: {}", path, problem);
        return false;
    }

    // the scene is replaced only once the whole document is understood
    scene->clear();
    for (const boost::shared_ptr<Item>& item : loaded.items) {
        scene->add(item);
    }
    scene->nextDocumentId(loaded.next);
    logger_->get()->info("read {} items from {}", scene->count(), path);
    return true;
}

/**
 **/
bool SceneStore::write(const std::string& path, const Scene& scene) const {
    boost::system::error_code error;
    const bool existed = boost::filesystem::exists(path, error);

    std::string problem;
    bool written = false;
    {
        v3d::database::Database database;
        bool fresh = false;
        if (!database.open(path, v3d::database::Database::Mode::Create)) {
            problem = database.error();
        } else if (header(&database, &fresh, &problem)) {
            v3d::database::Transaction transaction(&database);
            written = transaction.active() &&
                (!fresh || (database.execute(SCHEMA) && database.applicationId(APPLICATION_ID) &&
                    database.userVersion(SCHEMA_VERSION))) &&
                database.execute("DELETE FROM buffers; DELETE FROM items; DELETE FROM meta;") &&
                store(&database, scene, &problem) && transaction.commit();
            if (!written && problem.empty()) {
                problem = database.error();
            }
        }
    }

    if (!written) {
        // a document that this write made is not left behind empty
        if (!existed) {
            boost::filesystem::remove(path, error);
        }
        logger_->get()->error("Cannot write the scene {}: {}", path, problem);
        return false;
    }
    logger_->get()->info("wrote {} items to {}", scene.count(), path);
    return true;
}

};  // namespace v3d::editor
