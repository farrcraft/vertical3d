/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>
#include <api/brep/Index.h>

#include <array>
#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 * How a shaded view lights a mesh's surface: with each face's own normal, so the faces show,
 * or with a normal averaged at each vertex, so the surface looks curved.
 **/
enum class Shading : std::uint8_t { Flat, Smooth };

/**
 * A mesh as triangles a shaded pass draws: a position and a normal per vertex, and three
 * indices per triangle.
 *
 * A triangle winds counter-clockwise seen from outside, the reverse of a face's loop, because
 * that is the front face a pipeline culls by.
 **/
struct Triangles {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<std::uint32_t> indices;
};

/**
 * The triangles one face is cut into, as the mesh's vertex indices, each counter-clockwise
 * seen from outside.
 *
 * The face is cut by ear clipping in its own plane, so a concave face is cut into triangles
 * that stay inside it. A face of three corners is one triangle. A face that ear clipping cannot
 * cut, such as one whose points all lie on a line, is fanned from its first corner instead,
 * which covers no area either way. A face of fewer than three corners gives nothing.
 **/
std::vector<std::array<v3d::brep::Index, 3>> triangulateFace(const v3d::brep::BRep& mesh, v3d::brep::Index face);

/**
 * Every face of a mesh as triangles.
 *
 * Flat shading gives each face its own copy of its corners, with the face's normal. Smooth
 * shading gives each vertex of the mesh one copy, with the average of the normals of the faces
 * around it. A face whose stored normal is zero, as a ring an extrude makes has until it is
 * moved, takes its normal from its points, and +z when those have no area either.
 **/
Triangles triangulate(const v3d::brep::BRep& mesh, Shading shading);

};  // namespace v3d::editor
