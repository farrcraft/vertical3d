/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Triangulation.h"


#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "BRep.h"
#include "Face.h"
#include "HalfEdge.h"
#include "Index.h"
#include "Topology.h"
#include "Vertex.h"

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::brep {

namespace {

/**
 * A normal shorter than this has no direction.
 **/
constexpr float SHORTEST = 1.0e-6F;

/**
 * Twice the signed area of the triangle from an origin along one point and toward another,
 * positive when it winds counter-clockwise.
 **/
float turn(const glm::vec2& origin, const glm::vec2& along, const glm::vec2& toward) {
    return ((along.x - origin.x) * (toward.y - origin.y)) - ((along.y - origin.y) * (toward.x - origin.x));
}

/**
 * Whether a point is inside a counter-clockwise triangle or on its edge.
 **/
bool inside(const glm::vec2& point, const std::array<glm::vec2, 3>& triangle) {
    return turn(triangle[0], triangle[1], point) >= 0.0F && turn(triangle[1], triangle[2], point) >= 0.0F &&
        turn(triangle[2], triangle[0], point) >= 0.0F;
}

/**
 * The normal a face is lit by: its stored one, or one from its points when that is zero, or +z
 * when neither has a direction.
 **/
glm::vec3 faceNormal(const BRep& mesh, Index face) {
    const glm::vec3 stored = mesh.face(face)->normal();
    if (glm::length(stored) > SHORTEST) {
        return glm::normalize(stored);
    }
    const glm::vec3 found = newellNormal(mesh, face);
    if (glm::length(found) > SHORTEST) {
        return found;
    }
    return glm::vec3(0.0F, 0.0F, 1.0F);
}

/**
 * A face's corners counter-clockwise seen from outside, and where each lands in the face's own
 * plane.
 **/
struct Outline {
    std::vector<Index> corners;
    std::vector<glm::vec2> flat;
};

Outline outline(const BRep& mesh, Index face) {
    Outline result;
    // a half edge ends at a vertex, so the loop's ends are its corners in loop order. The loop
    // runs clockwise seen from outside, so the reverse is counter-clockwise
    for (const Index edge : faceLoop(mesh, face)) {
        result.corners.insert(result.corners.begin(), mesh.edge(edge)->vertex());
    }

    // the plane is seen from outside, so a counter-clockwise loop turns positive in it
    const glm::vec3 normal = faceNormal(mesh, face);
    const glm::vec3 helper = std::abs(normal.x) < 0.9F ? glm::vec3(1.0F, 0.0F, 0.0F) : glm::vec3(0.0F, 1.0F, 0.0F);
    const glm::vec3 u = glm::normalize(glm::cross(helper, normal));
    const glm::vec3 v = glm::cross(normal, u);
    for (const Index corner : result.corners) {
        const glm::vec3 point = mesh.vertex(corner)->point();
        result.flat.emplace_back(glm::dot(point, u), glm::dot(point, v));
    }
    return result;
}

/**
 * Where an ear is among the corners left: one that turns the right way and holds no other
 * corner inside the triangle it makes with its neighbours.
 **/
std::optional<std::size_t> ear(const Outline& shape, const std::vector<std::size_t>& left) {
    for (std::size_t index = 0; index < left.size(); index++) {
        const std::size_t before = left[(index + left.size() - 1) % left.size()];
        const std::size_t at = left[index];
        const std::size_t after = left[(index + 1) % left.size()];
        if (turn(shape.flat[before], shape.flat[at], shape.flat[after]) <= 0.0F) {
            continue;
        }
        const bool empty = std::none_of(left.begin(), left.end(), [&](std::size_t other) {
            return other != before && other != at && other != after &&
                inside(shape.flat[other], { shape.flat[before], shape.flat[at], shape.flat[after] });
        });
        if (empty) {
            return index;
        }
    }
    return std::nullopt;
}

/**
 * Every face as triangles sharing one copy of each vertex, lit by the faces around it.
 **/
Triangles smooth(const BRep& mesh) {
    Triangles result;
    std::vector<glm::vec3> sums(mesh.vertexCount(), glm::vec3(0.0F));
    std::vector<std::optional<glm::vec3>> firsts(mesh.vertexCount());
    for (Index face = 0; face < mesh.faceCount(); face++) {
        const glm::vec3 normal = faceNormal(mesh, face);
        for (const Index edge : faceLoop(mesh, face)) {
            const Index corner = mesh.edge(edge)->vertex();
            sums[corner] += normal;
            if (!firsts[corner]) {
                firsts[corner] = normal;
            }
        }
    }
    for (Index vertex = 0; vertex < mesh.vertexCount(); vertex++) {
        result.positions.push_back(mesh.vertex(vertex)->point());
        // faces that face opposite ways cancel, and the first of them lights the vertex
        const glm::vec3 first = firsts[vertex].value_or(glm::vec3(0.0F, 0.0F, 1.0F));
        result.normals.push_back(glm::length(sums[vertex]) > SHORTEST ? glm::normalize(sums[vertex]) : first);
    }
    for (Index face = 0; face < mesh.faceCount(); face++) {
        for (const std::array<Index, 3>& triangle : triangulateFace(mesh, face)) {
            result.indices.insert(result.indices.end(), triangle.begin(), triangle.end());
        }
    }
    return result;
}

/**
 * Every face as triangles with copies of its own corners, lit by that face alone. The face's
 * triangles share its copies.
 **/
Triangles flat(const BRep& mesh) {
    Triangles result;
    for (Index face = 0; face < mesh.faceCount(); face++) {
        const glm::vec3 normal = faceNormal(mesh, face);
        std::vector<std::pair<Index, std::uint32_t>> copies;
        for (const std::array<Index, 3>& triangle : triangulateFace(mesh, face)) {
            for (const Index corner : triangle) {
                const auto copy = std::find_if(copies.begin(), copies.end(),
                    [corner](const std::pair<Index, std::uint32_t>& made) { return made.first == corner; });
                if (copy != copies.end()) {
                    result.indices.push_back(copy->second);
                    continue;
                }
                const std::uint32_t index = static_cast<std::uint32_t>(result.positions.size());
                copies.emplace_back(corner, index);
                result.indices.push_back(index);
                result.positions.push_back(mesh.vertex(corner)->point());
                result.normals.push_back(normal);
            }
        }
    }
    return result;
}

};  // namespace

/**
 **/
std::vector<std::array<Index, 3>> triangulateFace(const BRep& mesh, Index face) {
    std::vector<std::array<Index, 3>> triangles;
    const Outline shape = outline(mesh, face);
    if (shape.corners.size() < 3) {
        return triangles;
    }

    // cut off an ear at a time until one triangle is left
    std::vector<std::size_t> left(shape.corners.size());
    for (std::size_t index = 0; index < left.size(); index++) {
        left[index] = index;
    }
    while (left.size() > 3) {
        const std::optional<std::size_t> found = ear(shape, left);
        if (!found) {
            // nothing turns the right way, so the face has no area to cut: fan what is left
            for (std::size_t index = 1; index + 1 < left.size(); index++) {
                triangles.push_back({ shape.corners[left[0]], shape.corners[left[index]], shape.corners[left[index + 1]] });
            }
            return triangles;
        }
        const std::size_t index = *found;
        triangles.push_back({ shape.corners[left[(index + left.size() - 1) % left.size()]], shape.corners[left[index]],
            shape.corners[left[(index + 1) % left.size()]] });
        left.erase(left.begin() + static_cast<std::ptrdiff_t>(index));
    }
    triangles.push_back({ shape.corners[left[0]], shape.corners[left[1]], shape.corners[left[2]] });
    return triangles;
}

/**
 **/
Triangles triangulate(const BRep& mesh, Normals normals) {
    return normals == Normals::Vertex ? smooth(mesh) : flat(mesh);
}

};  // namespace v3d::brep
