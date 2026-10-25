/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Modelling.h"

#include <api/brep/BRep.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>
#include <api/brep/Operations.h>
#include <api/brep/Topology.h>
#include <api/brep/Vertex.h>
#include <vertical3d/src/command/TopologyCommand.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include "Mesh.h"
#include "Scene.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

namespace {

/**
 * Report why an operation did nothing, if the caller asked.
 * @return an empty pointer, for the operation to return
 **/
boost::shared_ptr<TopologyCommand> refuse(std::string* problem, const std::string& why) {
    if (problem != nullptr) {
        *problem = why;
    }
    return boost::shared_ptr<TopologyCommand>();
}

/**
 * The geometry of every selected mesh item, in scene order. Two items may share one mesh,
 * which is listed once, so an operation never runs on it twice.
 **/
std::vector<boost::shared_ptr<v3d::brep::BRep>> selectedGeometry(const Scene& scene) {
    std::vector<boost::shared_ptr<v3d::brep::BRep>> geometry;
    for (const boost::shared_ptr<Mesh>& item : scene.selected<Mesh>()) {
        if (std::find(geometry.begin(), geometry.end(), item->geometry()) == geometry.end()) {
            geometry.push_back(item->geometry());
        }
    }
    return geometry;
}

/**
 * Run an operation on the selected faces of each selected mesh that has any, and record one
 * change for each.
 *
 * @return the change, already made, or an empty pointer when no face is selected
 **/
boost::shared_ptr<TopologyCommand> onSelectedFaces(const Scene& scene, const std::string& name,
    const std::function<void(v3d::brep::BRep*, const std::vector<v3d::brep::Index>&)>& operation) {
    std::vector<TopologyCommand::Change> changes;
    for (const boost::shared_ptr<v3d::brep::BRep>& mesh : selectedGeometry(scene)) {
        std::vector<v3d::brep::Index> faces;
        for (v3d::brep::Index id = 0; id < mesh->faceCount(); id++) {
            if (mesh->face(id)->selected()) {
                faces.push_back(id);
            }
        }
        if (faces.empty()) {
            continue;
        }
        TopologyCommand::Change change;
        change.mesh = mesh;
        change.before = mesh->snapshot();
        operation(mesh.get(), faces);
        change.after = mesh->snapshot();
        changes.push_back(change);
    }
    if (changes.empty()) {
        return boost::shared_ptr<TopologyCommand>();
    }
    return boost::make_shared<TopologyCommand>(changes, name);
}

};  // namespace

/**
 **/
boost::shared_ptr<TopologyCommand> splitEdges(const Scene& scene) {
    std::vector<TopologyCommand::Change> changes;
    for (const boost::shared_ptr<v3d::brep::BRep>& mesh : selectedGeometry(scene)) {
        // a selected edge is either of its halves, and is split once through the half that
        // stands for it
        std::vector<v3d::brep::Index> edges;
        for (v3d::brep::Index id = 0; id < mesh->edgeCount(); id++) {
            if (v3d::brep::ownsEdge(*mesh, id) && v3d::brep::edgeSelected(*mesh, id)) {
                edges.push_back(id);
            }
        }
        if (edges.empty()) {
            continue;
        }

        TopologyCommand::Change change;
        change.mesh = mesh;
        change.before = mesh->snapshot();
        mesh->deselectComponents();
        for (const v3d::brep::Index edge : edges) {
            if (!v3d::brep::splitEdge(*mesh, edge)) {
                continue;
            }
            // the half edge now ends at the new vertex, and the next one carries on beyond it
            mesh->edge(edge)->selected(true);
            mesh->edge(mesh->edge(edge)->next())->selected(true);
        }
        change.after = mesh->snapshot();
        changes.push_back(change);
    }
    if (changes.empty()) {
        return boost::shared_ptr<TopologyCommand>();
    }
    return boost::make_shared<TopologyCommand>(changes, "split edge");
}

/**
 **/
boost::shared_ptr<TopologyCommand> splitFace(const Scene& scene, std::string* problem) {
    boost::shared_ptr<v3d::brep::BRep> owner;
    std::vector<v3d::brep::Index> vertices;
    for (const boost::shared_ptr<v3d::brep::BRep>& mesh : selectedGeometry(scene)) {
        for (v3d::brep::Index id = 0; id < mesh->vertexCount(); id++) {
            if (!mesh->vertex(id)->selected()) {
                continue;
            }
            if (owner && owner != mesh) {
                return refuse(problem, "the two vertices are in different meshes");
            }
            owner = mesh;
            vertices.push_back(id);
        }
    }
    if (vertices.size() != 2) {
        return refuse(problem, "it needs exactly two vertices selected, and " + std::to_string(vertices.size()) + " are");
    }

    // any face both are corners of will do, and the first that can be split is
    const std::vector<v3d::brep::Index> firstFaces = v3d::brep::facesAround(*owner, vertices[0]);
    const std::vector<v3d::brep::Index> secondFaces = v3d::brep::facesAround(*owner, vertices[1]);
    std::vector<v3d::brep::Index> shared;
    std::set_intersection(firstFaces.begin(), firstFaces.end(), secondFaces.begin(), secondFaces.end(),
        std::back_inserter(shared));

    const v3d::brep::BRep::Snapshot before = owner->snapshot();
    for (const v3d::brep::Index face : shared) {
        if (v3d::brep::splitFace(*owner, face, vertices[0], vertices[1])) {
            const TopologyCommand::Change change{.mesh = owner, .before = before, .after = owner->snapshot()};
            return boost::make_shared<TopologyCommand>(std::vector<TopologyCommand::Change>{change}, "split face");
        }
    }
    return refuse(problem, shared.empty() ? "the two vertices are not corners of one face" :
        "the two vertices are next to each other on every face they share");
}

/**
 **/
boost::shared_ptr<TopologyCommand> extrudeFaces(const Scene& scene) {
    // each face keeps its index, and with it its selection flag
    return onSelectedFaces(scene, "extrude face", [](v3d::brep::BRep* mesh, const std::vector<v3d::brep::Index>& faces) {
        v3d::brep::extrudeFaces(*mesh, faces);
    });
}

/**
 **/
boost::shared_ptr<TopologyCommand> insetFaces(const Scene& scene) {
    return onSelectedFaces(scene, "inset face", [](v3d::brep::BRep* mesh, const std::vector<v3d::brep::Index>& faces) {
        v3d::brep::insetFaces(*mesh, faces, insetFraction);
    });
}

/**
 **/
boost::shared_ptr<TopologyCommand> mergeVertices(const Scene& scene, std::string* problem) {
    boost::shared_ptr<v3d::brep::BRep> owner;
    std::vector<v3d::brep::Index> vertices;
    for (const boost::shared_ptr<v3d::brep::BRep>& mesh : selectedGeometry(scene)) {
        for (v3d::brep::Index id = 0; id < mesh->vertexCount(); id++) {
            if (!mesh->vertex(id)->selected()) {
                continue;
            }
            if (owner && owner != mesh) {
                return refuse(problem, "the vertices are in different meshes");
            }
            owner = mesh;
            vertices.push_back(id);
        }
    }
    if (vertices.size() < 2) {
        return refuse(problem, "it needs at least two vertices selected, and " + std::to_string(vertices.size()) + " are");
    }

    const v3d::brep::BRep::Snapshot before = owner->snapshot();
    if (!v3d::brep::mergeVertices(*owner, vertices)) {
        return refuse(problem, "it would leave an edge shared by more than two faces, or a face that meets itself");
    }
    const TopologyCommand::Change change{.mesh = owner, .before = before, .after = owner->snapshot()};
    return boost::make_shared<TopologyCommand>(std::vector<TopologyCommand::Change>{change}, "merge vertices");
}

};  // namespace v3d::editor
