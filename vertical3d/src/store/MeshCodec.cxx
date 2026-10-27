/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MeshCodec.h"

#include <api/brep/BRep.h>
#include <api/brep/Face.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>
#include <api/brep/Vertex.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Mesh.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "Bytes.h"
#include "Codec.h"
#include "Payload.h"

#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

namespace {

/**
 * The size of one entry of each of a mesh's three blobs. A vertex is its point. A half edge is
 * its vertex, face, pair and next. A face is its normal and its first half edge.
 **/
constexpr std::size_t VERTEX_SIZE = 3 * sizeof(float);
constexpr std::size_t EDGE_SIZE = 4 * sizeof(std::uint32_t);
constexpr std::size_t FACE_SIZE = (3 * sizeof(float)) + sizeof(std::uint32_t);

/**
 * Build a mesh from its three blobs, index for index.
 **/
bool decode(const Blobs& blobs, v3d::brep::BRep* mesh, std::string* problem) {
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

};  // namespace

/**
 **/
Item::Kind MeshCodec::kind() const noexcept {
    return Item::Kind::Mesh;
}

/**
 **/
std::string_view MeshCodec::name() const noexcept {
    return "mesh";
}

/**
 **/
std::int64_t MeshCodec::version() const noexcept {
    return VERSION;
}

/**
 **/
bool MeshCodec::buffered() const noexcept {
    return true;
}

/**
 **/
boost::json::object MeshCodec::payload(const Item& /*item*/) const {
    return boost::json::object();
}

/**
 **/
std::vector<Buffer> MeshCodec::buffers(const Item& item) const {
    const v3d::brep::BRep& mesh = *static_cast<const Mesh&>(item).geometry();
    Buffer vertices{ .role = "vertices", .count = mesh.vertexCount() };
    for (v3d::brep::Index id = 0; id < mesh.vertexCount(); id++) {
        put(&vertices.data, mesh.vertex(id)->point());
    }
    Buffer edges{ .role = "edges", .count = mesh.edgeCount() };
    for (v3d::brep::Index id = 0; id < mesh.edgeCount(); id++) {
        const v3d::brep::HalfEdge* edge = mesh.edge(id);
        put(&edges.data, edge->vertex());
        put(&edges.data, edge->face());
        put(&edges.data, edge->pair());
        put(&edges.data, edge->next());
    }
    Buffer faces{ .role = "faces", .count = mesh.faceCount() };
    for (v3d::brep::Index id = 0; id < mesh.faceCount(); id++) {
        const v3d::brep::Face* face = mesh.face(id);
        put(&faces.data, face->normal());
        put(&faces.data, face->edge());
    }
    return { vertices, edges, faces };
}

/**
 **/
boost::shared_ptr<Item> MeshCodec::read(const boost::json::object& payload, const Blobs& blobs, std::string* problem) const {
    if (!exactly(payload, {}, problem)) {
        return nullptr;
    }
    const boost::shared_ptr<v3d::brep::BRep> geometry = boost::make_shared<v3d::brep::BRep>();
    if (!decode(blobs, geometry.get(), problem)) {
        return nullptr;
    }
    return boost::make_shared<Mesh>(geometry);
}

};  // namespace v3d::editor
