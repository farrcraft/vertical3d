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

};  // namespace v3d::editor
