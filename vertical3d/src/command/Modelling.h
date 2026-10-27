/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once


#include <vertical3d/src/scene/Scene.h>

#include <string>

#include "TopologyCommand.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * Split every selected edge of every selected mesh at its middle.
 *
 * The two parts of each split edge are selected after it, so the operation can be repeated.
 *
 * @return the change, already made, or an empty pointer when no edge is selected
 **/
boost::shared_ptr<TopologyCommand> splitEdges(const boost::shared_ptr<Scene>& scene);

/**
 * Split a face in two with a new edge between the two selected vertices.
 *
 * Exactly two vertices must be selected, in one mesh, and they must be corners of one face
 * that are not next to each other on it. The two vertices stay selected.
 *
 * @param problem why nothing was split, when nothing was
 * @return the change, already made, or an empty pointer when nothing was split
 **/
boost::shared_ptr<TopologyCommand> splitFace(const boost::shared_ptr<Scene>& scene, std::string* problem);

/**
 * Extrude the selected faces of every selected mesh. Faces that share an edge extrude as one
 * region.
 *
 * The faces stay selected and have not moved, so a manipulator can pull them out.
 *
 * @return the change, already made, or an empty pointer when no face is selected
 **/
boost::shared_ptr<TopologyCommand> extrudeFaces(const boost::shared_ptr<Scene>& scene);

/**
 * How far an inset moves each corner of a face toward the face's centre.
 **/
constexpr float insetFraction = 0.25F;

/**
 * Inset each selected face of every selected mesh on its own, by insetFraction.
 *
 * The smaller faces stay selected. The handles pivot at a face's centre, so the scale
 * manipulator changes how far it is inset.
 *
 * @return the change, already made, or an empty pointer when no face is selected
 **/
boost::shared_ptr<TopologyCommand> insetFaces(const boost::shared_ptr<Scene>& scene);

/**
 * Merge the selected vertices into one at their centre.
 *
 * At least two vertices must be selected, in one mesh. The mesh is renumbered, and the merged
 * vertex stays selected. A merge that would leave an edge shared by more than two faces, or a
 * face that meets itself, is refused.
 *
 * @param problem why nothing was merged, when nothing was
 * @return the change, already made, or an empty pointer when nothing was merged
 **/
boost::shared_ptr<TopologyCommand> mergeVertices(const boost::shared_ptr<Scene>& scene, std::string* problem);

};  // namespace v3d::editor
