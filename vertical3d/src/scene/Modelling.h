/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/command/TopologyCommand.h>

#include <string>

#include "Scene.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * Split every selected edge of every selected mesh at its middle.
 *
 * The two parts of each split edge are selected after it, so the operation can be repeated.
 *
 * @return the change, already made, or an empty pointer when no edge is selected
 **/
boost::shared_ptr<TopologyCommand> splitEdges(const Scene& scene);

/**
 * Split a face in two with a new edge between the two selected vertices.
 *
 * Exactly two vertices must be selected, in one mesh, and they must be corners of one face
 * that are not next to each other on it. The two vertices stay selected.
 *
 * @param problem why nothing was split, when nothing was
 * @return the change, already made, or an empty pointer when nothing was split
 **/
boost::shared_ptr<TopologyCommand> splitFace(const Scene& scene, std::string* problem);

/**
 * Extrude the selected faces of every selected mesh. Faces that share an edge extrude as one
 * region.
 *
 * The faces stay selected and have not moved, so a manipulator can pull them out.
 *
 * @return the change, already made, or an empty pointer when no face is selected
 **/
boost::shared_ptr<TopologyCommand> extrudeFaces(const Scene& scene);

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
boost::shared_ptr<TopologyCommand> insetFaces(const Scene& scene);

};  // namespace v3d::editor
