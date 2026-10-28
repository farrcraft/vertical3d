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
#include <vertical3d/src/scene/Components.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "TopologyCommand.h"

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
 * Every selected mesh item whose geometry no earlier one shows, in scene order. Two items may
 * share one mesh, and an operation runs on it once, with the selection of the first of them.
 **/
Meshes selectedMeshes(const Scene& scene) {
    Meshes meshes;
    for (const boost::shared_ptr<Mesh>& item : scene.selected<Mesh>()) {
        const bool seen = std::any_of(meshes.begin(), meshes.end(),
            [&item](const boost::shared_ptr<Mesh>& other) { return other->geometry() == item->geometry(); });
        if (!seen) {
            meshes.push_back(item);
        }
    }
    return meshes;
}

/**
 * The selected parts of one kind of a mesh item that its geometry holds, rising.
 **/
std::vector<v3d::brep::Index> selectedParts(const Scene& scene, const Mesh& item, Component kind, std::size_t count) {
    std::vector<v3d::brep::Index> parts = scene.components().of(item.id(), kind);
    std::erase_if(parts, [count](v3d::brep::Index index) { return index >= count; });
    return parts;
}

/**
 * Give one item a new selection of one kind of part after its geometry has changed. Every
 * other item that shows the geometry loses its component selection, since an index it held
 * could name another part now.
 **/
void reselect(Scene* scene, const Mesh& item, Component kind, const std::vector<v3d::brep::Index>& parts) {
    scene->deselectComponents(item.geometry().get());
    for (const v3d::brep::Index part : parts) {
        scene->components().select(item.id(), kind, part, true);
    }
}

/**
 * Run an operation on the selected faces of each selected mesh that has any, and record one
 * change for each. The operation keeps the faces' indices, so the faces stay selected.
 *
 * @return the change, already made, or an empty pointer when no face is selected
 **/
boost::shared_ptr<TopologyCommand> onSelectedFaces(const boost::shared_ptr<Scene>& scene, const std::string& name,
    const std::function<void(v3d::brep::BRep*, const std::vector<v3d::brep::Index>&)>& operation) {
    std::vector<TopologyCommand::Change> changes;
    for (const boost::shared_ptr<Mesh>& item : selectedMeshes(*scene)) {
        const boost::shared_ptr<v3d::brep::BRep>& mesh = item->geometry();
        const std::vector<v3d::brep::Index> faces = selectedParts(*scene, *item, Component::Face, mesh->faceCount());
        if (faces.empty()) {
            continue;
        }
        TopologyCommand::Change change;
        change.mesh = mesh;
        change.before = mesh->snapshot();
        operation(mesh.get(), faces);
        change.after = mesh->snapshot();
        changes.push_back(change);
        reselect(scene.get(), *item, Component::Face, faces);
    }
    if (changes.empty()) {
        return boost::shared_ptr<TopologyCommand>();
    }
    return boost::make_shared<TopologyCommand>(scene, changes, name);
}

/**
 * The one mesh item whose selected vertices an operation works on, and those vertices.
 *
 * @return false when vertices of two meshes are selected
 **/
bool selectedVertices(const Scene& scene, boost::shared_ptr<Mesh>* owner, std::vector<v3d::brep::Index>* vertices) {
    std::vector<std::pair<boost::shared_ptr<Mesh>, std::vector<v3d::brep::Index>>> found;
    for (const boost::shared_ptr<Mesh>& item : selectedMeshes(scene)) {
        std::vector<v3d::brep::Index> parts =
            selectedParts(scene, *item, Component::Vertex, item->geometry()->vertexCount());
        if (!parts.empty()) {
            found.emplace_back(item, std::move(parts));
        }
    }
    if (found.size() > 1) {
        return false;
    }
    if (!found.empty()) {
        *owner = found.front().first;
        *vertices = found.front().second;
    }
    return true;
}

};  // namespace

/**
 **/
boost::shared_ptr<TopologyCommand> splitEdges(const boost::shared_ptr<Scene>& scene) {
    std::vector<TopologyCommand::Change> changes;
    for (const boost::shared_ptr<Mesh>& item : selectedMeshes(*scene)) {
        const boost::shared_ptr<v3d::brep::BRep>& mesh = item->geometry();
        // a selected edge is either of its halves, and is split once through the half that
        // stands for it
        std::vector<v3d::brep::Index> edges;
        for (v3d::brep::Index id = 0; id < mesh->edgeCount(); id++) {
            if (v3d::brep::ownsEdge(*mesh, id) && scene->components().edge(item->id(), *mesh, id)) {
                edges.push_back(id);
            }
        }
        if (edges.empty()) {
            continue;
        }

        TopologyCommand::Change change;
        change.mesh = mesh;
        change.before = mesh->snapshot();
        std::vector<v3d::brep::Index> halves;
        for (const v3d::brep::Index edge : edges) {
            if (!v3d::brep::splitEdge(*mesh, edge)) {
                continue;
            }
            // the half edge now ends at the new vertex, and the next one carries on beyond it
            halves.push_back(edge);
            halves.push_back(mesh->edge(edge)->next());
        }
        change.after = mesh->snapshot();
        changes.push_back(change);
        reselect(scene.get(), *item, Component::Edge, halves);
    }
    if (changes.empty()) {
        return boost::shared_ptr<TopologyCommand>();
    }
    return boost::make_shared<TopologyCommand>(scene, changes, "split edge");
}

/**
 **/
boost::shared_ptr<TopologyCommand> splitFace(const boost::shared_ptr<Scene>& scene, std::string* problem) {
    boost::shared_ptr<Mesh> item;
    std::vector<v3d::brep::Index> vertices;
    if (!selectedVertices(*scene, &item, &vertices)) {
        return refuse(problem, "the two vertices are in different meshes");
    }
    if (vertices.size() != 2) {
        return refuse(problem, "it needs exactly two vertices selected, and " + std::to_string(vertices.size()) + " are");
    }

    // any face both are corners of will do, and the first that can be split is
    const boost::shared_ptr<v3d::brep::BRep>& owner = item->geometry();
    const std::vector<v3d::brep::Index> firstFaces = v3d::brep::facesAround(*owner, vertices[0]);
    const std::vector<v3d::brep::Index> secondFaces = v3d::brep::facesAround(*owner, vertices[1]);
    std::vector<v3d::brep::Index> shared;
    std::set_intersection(firstFaces.begin(), firstFaces.end(), secondFaces.begin(), secondFaces.end(),
        std::back_inserter(shared));

    const v3d::brep::BRep::Snapshot before = owner->snapshot();
    for (const v3d::brep::Index face : shared) {
        if (v3d::brep::splitFace(*owner, face, vertices[0], vertices[1])) {
            const TopologyCommand::Change change{.mesh = owner, .before = before, .after = owner->snapshot()};
            // a split adds parts and renumbers none, so the two vertices stay selected
            reselect(scene.get(), *item, Component::Vertex, vertices);
            return boost::make_shared<TopologyCommand>(scene, std::vector<TopologyCommand::Change>{change}, "split face");
        }
    }
    return refuse(problem, shared.empty() ? "the two vertices are not corners of one face" :
        "the two vertices are next to each other on every face they share");
}

/**
 **/
boost::shared_ptr<TopologyCommand> extrudeFaces(const boost::shared_ptr<Scene>& scene) {
    return onSelectedFaces(scene, "extrude face", [](v3d::brep::BRep* mesh, const std::vector<v3d::brep::Index>& faces) {
        v3d::brep::extrudeFaces(*mesh, faces);
    });
}

/**
 **/
boost::shared_ptr<TopologyCommand> insetFaces(const boost::shared_ptr<Scene>& scene) {
    return onSelectedFaces(scene, "inset face", [](v3d::brep::BRep* mesh, const std::vector<v3d::brep::Index>& faces) {
        v3d::brep::insetFaces(*mesh, faces, insetFraction);
    });
}

/**
 **/
boost::shared_ptr<TopologyCommand> mergeVertices(const boost::shared_ptr<Scene>& scene, std::string* problem) {
    boost::shared_ptr<Mesh> item;
    std::vector<v3d::brep::Index> vertices;
    if (!selectedVertices(*scene, &item, &vertices)) {
        return refuse(problem, "the vertices are in different meshes");
    }
    if (vertices.size() < 2) {
        return refuse(problem, "it needs at least two vertices selected, and " + std::to_string(vertices.size()) + " are");
    }

    const boost::shared_ptr<v3d::brep::BRep>& owner = item->geometry();
    const v3d::brep::BRep::Snapshot before = owner->snapshot();
    const std::optional<v3d::brep::Index> merged = v3d::brep::mergeVertices(*owner, vertices);
    if (!merged) {
        return refuse(problem, "it would leave an edge shared by more than two faces, or a face that meets itself");
    }
    const TopologyCommand::Change change{.mesh = owner, .before = before, .after = owner->snapshot()};
    // the merge renumbers the mesh, so the merged vertex is all that is selected after it
    reselect(scene.get(), *item, Component::Vertex, { *merged });
    return boost::make_shared<TopologyCommand>(scene, std::vector<TopologyCommand::Change>{change}, "merge vertices");
}

};  // namespace v3d::editor
