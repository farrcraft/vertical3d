/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <vector>

#include "../../../api/brep/BRep.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

    /**
     * The half edges of one face, in order.
     *
     * A half edge names the next edge round its face, so a face's geometry is reached by
     * walking the chain - there is no array of a face's vertices to index into.
     *
     * Bounded by the mesh's edge count, so a next chain that does not close - which is what
     * an unfinished modelling operation leaves behind - ends the walk rather than spinning.
     *
     * @return the half edge ids, or nothing if the face does not exist
     **/
    std::vector<unsigned int> faceLoop(const boost::shared_ptr<v3d::brep::BRep>& mesh, unsigned int face);

    /**
     * The segment one entry of a face loop draws.
     *
     * A half edge names the vertex it ends at, so its segment starts where the one before
     * it in the loop ended.
     *
     * @param loop what faceLoop() returned
     * @param index which entry of it
     * @param from where the segment starts - may be null
     * @param to where it ends - may be null
     * @return whether both endpoints could be resolved
     **/
    bool loopSegment(const boost::shared_ptr<v3d::brep::BRep>& mesh, const std::vector<unsigned int>& loop,
        std::size_t index, glm::vec3* from, glm::vec3* to);

    /**
     * Whether this entry of a loop is the half of its edge that draws it.
     *
     * A half edge and its pair are the same edge seen from the two faces that share it, so
     * the lower numbered of the two owns it and the other is skipped. An unpaired edge owns
     * itself.
     **/
    bool ownsEdge(const boost::shared_ptr<v3d::brep::BRep>& mesh, unsigned int edge);

};  // namespace v3d::editor
