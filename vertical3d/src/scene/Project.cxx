/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Project.h"

#include <api/asset/kind/JsonFile.h>
#include <api/asset/Writer.h>
#include <api/brep/BRep.h>
#include <api/brep/Face.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Vertex.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "SceneVisitor.h"

#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace v3d::editor {

namespace {

/**
 * Read an array of n numbers.
 * @return false when the value is not that array, leaving out alone
 **/
bool numbers(const boost::json::value& value, std::size_t count, float* out) {
    if (!value.is_array()) {
        return false;
    }
    const boost::json::array& values = value.as_array();
    if (values.size() != count) {
        return false;
    }
    for (std::size_t item = 0; item < count; item++) {
        if (!values[item].is_number()) {
            return false;
        }
        out[item] = static_cast<float>(values[item].to_number<double>());
    }
    return true;
}

/**
 * @return false when the entry is missing or is not that array, leaving out alone
 **/
bool numbers(const boost::json::object& entry, const char* key, std::size_t count, float* out) {
    return entry.contains(key) && numbers(entry.at(key), count, out);
}

/**
 * Read a whole number, which is what every reference within a mesh is.
 * A double, a negative, or one too large for brep::Index is a malformed index
 * rather than one to round or truncate.
 * @return false when the entry is missing or is not one, leaving out alone
 **/
bool index(const boost::json::object& entry, const char* key, v3d::brep::Index* out) {
    if (!entry.contains(key)) {
        return false;
    }
    const boost::json::value& value = entry.at(key);
    uint64_t whole = 0;
    if (value.is_uint64()) {
        whole = value.as_uint64();
    } else if (value.is_int64() && value.as_int64() >= 0) {
        whole = static_cast<uint64_t>(value.as_int64());
    } else {
        return false;
    }
    if (whole > std::numeric_limits<v3d::brep::Index>::max()) {
        return false;
    }
    *out = static_cast<v3d::brep::Index>(whole);
    return true;
}

/**
 * Whether a reference names something the mesh holds. INVALID_ID is allowed
 * wherever a reference may be absent - an edge on a boundary has no pair.
 **/
bool refers(v3d::brep::Index id, std::size_t count, bool optional) {
    if (optional && id == v3d::brep::INVALID_ID) {
        return true;
    }
    return id < count;
}

/**
 **/
boost::json::array vector(const glm::vec3& v) {
    return boost::json::array{ v.x, v.y, v.z };
}

/**
 * Collects every mesh of a scene as the object the file stores it as.
 **/
class WriteVisitor final : public SceneVisitor {
 public:
    void visit(const boost::shared_ptr<v3d::brep::BRep>& mesh) override {
        if (!mesh) {
            return;
        }
        boost::json::object transform;
        transform["translation"] = vector(mesh->translation());
        const glm::quat rotation = mesh->rotation();
        transform["rotation"] = boost::json::array{ rotation.x, rotation.y, rotation.z, rotation.w };
        transform["scale"] = vector(mesh->scale());

        boost::json::array vertices;
        for (std::size_t id = 0; id < mesh->vertexCount(); id++) {
            vertices.push_back(vector(mesh->vertex(static_cast<unsigned int>(id))->point()));
        }

        boost::json::array edges;
        for (std::size_t id = 0; id < mesh->edgeCount(); id++) {
            const v3d::brep::HalfEdge* edge = mesh->edge(static_cast<unsigned int>(id));
            boost::json::object entry;
            entry["vertex"] = edge->vertex();
            entry["face"] = edge->face();
            entry["pair"] = edge->pair();
            entry["next"] = edge->next();
            edges.push_back(entry);
        }

        boost::json::array faces;
        for (std::size_t id = 0; id < mesh->faceCount(); id++) {
            const v3d::brep::Face* face = mesh->face(static_cast<unsigned int>(id));
            boost::json::object entry;
            entry["normal"] = vector(face->normal());
            entry["edge"] = face->edge();
            faces.push_back(entry);
        }

        boost::json::object entry;
        entry["transform"] = transform;
        entry["vertices"] = vertices;
        entry["edges"] = edges;
        entry["faces"] = faces;
        meshes.push_back(entry);
    }

    boost::json::array meshes;
};

/**
 * Read the placement a mesh was saved under. Every part of it is optional: a mesh saved
 * before the transform was written carries none, and is left where a new one starts.
 **/
void readTransform(const boost::json::object& entry, const boost::shared_ptr<v3d::brep::BRep>& mesh) {
    if (!entry.contains("transform") || !entry.at("transform").is_object()) {
        return;
    }
    const boost::json::object& transform = entry.at("transform").as_object();
    float values[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (numbers(transform, "translation", 3, values)) {
        mesh->translation(glm::vec3(values[0], values[1], values[2]));
    }
    if (numbers(transform, "scale", 3, values)) {
        mesh->scale(glm::vec3(values[0], values[1], values[2]));
    }
    if (numbers(transform, "rotation", 4, values)) {
        mesh->rotation(glm::quat(values[3], values[0], values[1], values[2]));
    }
}

bool readVertices(const boost::json::array& points, const std::string& path,
    const boost::shared_ptr<v3d::brep::BRep>& mesh, const boost::shared_ptr<v3d::log::Logger>& logger) {
    for (const boost::json::value& point : points) {
        float values[3] = { 0.0f, 0.0f, 0.0f };
        if (!numbers(point, 3, values)) {
            logger->get()->error("A vertex in {} is not three numbers", path);
            return false;
        }
        mesh->addVertex(v3d::brep::Vertex(glm::vec3(values[0], values[1], values[2])));
    }
    return true;
}

bool readEdges(const boost::json::array& edges, const std::string& path,
    const boost::shared_ptr<v3d::brep::BRep>& mesh, const boost::shared_ptr<v3d::log::Logger>& logger) {
    for (const boost::json::value& edgeEntry : edges) {
        if (!edgeEntry.is_object()) {
            logger->get()->error("An edge in {} is not an object", path);
            return false;
        }
        const boost::json::object& record = edgeEntry.as_object();
        v3d::brep::Index vertex = 0;
        v3d::brep::Index face = 0;
        v3d::brep::Index pair = 0;
        v3d::brep::Index next = 0;
        if (!index(record, "vertex", &vertex) || !index(record, "face", &face) ||
            !index(record, "pair", &pair) || !index(record, "next", &next)) {
            logger->get()->error("An edge in {} is missing one of its references", path);
            return false;
        }
        v3d::brep::HalfEdge edge;
        edge.vertex(vertex);
        edge.face(face);
        edge.pair(pair);
        edge.next(next);
        mesh->addEdge(edge);
    }
    return true;
}

bool readFaces(const boost::json::array& faces, const std::string& path,
    const boost::shared_ptr<v3d::brep::BRep>& mesh, const boost::shared_ptr<v3d::log::Logger>& logger) {
    for (const boost::json::value& faceEntry : faces) {
        if (!faceEntry.is_object()) {
            logger->get()->error("A face in {} is not an object", path);
            return false;
        }
        const boost::json::object& record = faceEntry.as_object();
        float normal[3] = { 0.0f, 0.0f, 0.0f };
        v3d::brep::Index edge = 0;
        if (!numbers(record, "normal", 3, normal) || !index(record, "edge", &edge)) {
            logger->get()->error("A face in {} is missing its normal or its edge", path);
            return false;
        }
        mesh->addFace(v3d::brep::Face(glm::vec3(normal[0], normal[1], normal[2]),
            static_cast<unsigned int>(edge)));
    }
    return true;
}

/**
 * A reference out of range is a mesh the wireframe and the picker would walk off the end
 * of, so it is refused here rather than found by whatever reads it first.
 **/
bool validMesh(const boost::shared_ptr<v3d::brep::BRep>& mesh, const std::string& path,
    const boost::shared_ptr<v3d::log::Logger>& logger) {
    for (std::size_t id = 0; id < mesh->edgeCount(); id++) {
        const v3d::brep::HalfEdge* edge = mesh->edge(static_cast<unsigned int>(id));
        if (!refers(edge->vertex(), mesh->vertexCount(), false) ||
            !refers(edge->face(), mesh->faceCount(), true) ||
            !refers(edge->pair(), mesh->edgeCount(), true) ||
            !refers(edge->next(), mesh->edgeCount(), true)) {
            logger->get()->error("Edge {} in {} names something the mesh does not hold", id, path);
            return false;
        }
    }
    for (std::size_t id = 0; id < mesh->faceCount(); id++) {
        if (!refers(mesh->face(static_cast<unsigned int>(id))->edge(), mesh->edgeCount(), false)) {
            logger->get()->error("Face {} in {} names an edge the mesh does not hold", id, path);
            return false;
        }
    }
    return true;
}

/**
 * @return the mesh one entry of the meshes array describes, or null when it does not
 *         describe a consistent one
 **/
boost::shared_ptr<v3d::brep::BRep> readMesh(const boost::json::object& entry, const std::string& path,
    const boost::shared_ptr<v3d::log::Logger>& logger) {
    boost::shared_ptr<v3d::brep::BRep> mesh = boost::make_shared<v3d::brep::BRep>();
    readTransform(entry, mesh);

    if (!entry.contains("vertices") || !entry.at("vertices").is_array() ||
        !entry.contains("edges") || !entry.at("edges").is_array() ||
        !entry.contains("faces") || !entry.at("faces").is_array()) {
        logger->get()->error("A mesh in {} is missing its vertices, edges or faces", path);
        return nullptr;
    }

    if (!readVertices(entry.at("vertices").as_array(), path, mesh, logger) ||
        !readEdges(entry.at("edges").as_array(), path, mesh, logger) ||
        !readFaces(entry.at("faces").as_array(), path, mesh, logger) ||
        !validMesh(mesh, path, logger)) {
        return nullptr;
    }
    return mesh;
}

};  // namespace

/**
 **/
const int Project::VERSION = 1;

/**
 **/
Project::Project(const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger),
    name_("untitled") {
}

/**
 **/
const std::string& Project::name() const noexcept {
    return name_;
}

/**
 **/
void Project::name(const std::string& title) {
    name_ = title;
}

/**
 **/
bool Project::read(const std::string& path, const boost::shared_ptr<Scene>& scene) {
    if (!scene) {
        return false;
    }

    const std::string text = v3d::asset::kind::read_file(path.c_str());
    if (text.empty()) {
        logger_->get()->error("No project to read at {}", path);
        return false;
    }

    boost::system::error_code error;
    const boost::json::value document = boost::json::parse(text, error);
    if (error || !document.is_object()) {
        logger_->get()->error("{} is not a project: {}", path, error.message());
        return false;
    }
    const boost::json::object& root = document.as_object();

    v3d::brep::Index version = 0;
    if (!index(root, "version", &version) || version != static_cast<v3d::brep::Index>(VERSION)) {
        logger_->get()->error("{} is not a version {} project", path, VERSION);
        return false;
    }
    if (!root.contains("meshes") || !root.at("meshes").is_array()) {
        logger_->get()->error("{} has no meshes", path);
        return false;
    }

    // built to one side and moved into the scene only once the whole file has been
    // understood, so a file that goes wrong half way through does not half replace a
    // document that was fine
    std::vector<boost::shared_ptr<v3d::brep::BRep>> meshes;
    for (const boost::json::value& value : root.at("meshes").as_array()) {
        if (!value.is_object()) {
            logger_->get()->error("{} holds something that is not a mesh", path);
            return false;
        }
        boost::shared_ptr<v3d::brep::BRep> mesh = readMesh(value.as_object(), path, logger_);
        if (!mesh) {
            return false;
        }
        meshes.push_back(mesh);
    }

    name_ = root.contains("name") && root.at("name").is_string() ?
        boost::json::value_to<std::string>(root.at("name")) : std::string("untitled");

    scene->clear();
    for (const boost::shared_ptr<v3d::brep::BRep>& mesh : meshes) {
        scene->add(mesh);
    }
    logger_->get()->info("read {} - {} meshes from {}", name_, scene->count(), path);
    return true;
}

/**
 **/
bool Project::write(const std::string& path, const boost::shared_ptr<Scene>& scene) const {
    if (!scene) {
        return false;
    }

    WriteVisitor visitor;
    scene->accept(&visitor);

    boost::json::object root;
    root["version"] = VERSION;
    root["name"] = name_;
    root["meshes"] = visitor.meshes;

    if (!v3d::asset::writeDocument(path, root)) {
        logger_->get()->error("Failed writing the project to {}", path);
        return false;
    }
    logger_->get()->info("wrote {} - {} meshes to {}", name_, scene->count(), path);
    return true;
}

};  // namespace v3d::editor
