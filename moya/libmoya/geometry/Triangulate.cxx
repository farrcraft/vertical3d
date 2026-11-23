/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/Triangulate.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::geometry {

namespace {

/** Twice the signed area of a triangle in the plane, positive when it turns anticlockwise. **/
float turn(const glm::vec2 & p0, const glm::vec2 & p1, const glm::vec2 & p2) {
    return ((p1.x - p0.x) * (p2.y - p0.y)) - ((p1.y - p0.y) * (p2.x - p0.x));
}

/** Whether a point lies inside an anticlockwise triangle or on its edges. **/
bool inside(const glm::vec2 & point, const glm::vec2 & p0, const glm::vec2 & p1, const glm::vec2 & p2) {
    return turn(p0, p1, point) >= 0.0F && turn(p1, p2, point) >= 0.0F && turn(p2, p0, point) >= 0.0F;
}

/**
 * A face's normal by Newell's method, which is the plane a face that is not planar is nearest.
 * Its length is twice the face's area.
 **/
glm::vec3 newell(const std::vector<glm::vec3> & corners) {
    glm::vec3 normal { 0.0F };
    for (std::size_t i = 0; i < corners.size(); ++i) {
        const glm::vec3 & here = corners[i];
        const glm::vec3 & next = corners[(i + 1) % corners.size()];
        normal.x += (here.y - next.y) * (here.z + next.z);
        normal.y += (here.z - next.z) * (here.x + next.x);
        normal.z += (here.x - next.x) * (here.y + next.y);
    }
    return normal;
}

/**
 * The face's points projected onto the plane of the two axes its normal is shortest along, turned
 * so the face runs anticlockwise.
 **/
std::vector<glm::vec2> projected(const std::vector<glm::vec3> & corners, const glm::vec3 & normal) {
    const glm::vec3 size { std::fabs(normal.x), std::fabs(normal.y), std::fabs(normal.z) };
    int u = 0;
    int v = 1;
    int w = 2;
    if (size.x >= size.y && size.x >= size.z) {
        u = 1;
        v = 2;
        w = 0;
    } else if (size.y >= size.z) {
        u = 2;
        v = 0;
        w = 1;
    }
    // looking down the normal's longest axis from its positive side, u then v turns anticlockwise
    const float flip = normal[w] < 0.0F ? -1.0F : 1.0F;
    std::vector<glm::vec2> out;
    out.reserve(corners.size());
    for (const glm::vec3 & corner : corners) {
        out.emplace_back(corner[u] * flip, corner[v]);
    }
    return out;
}

/**
 * Whether the corner at position at of the remaining ones is an ear: convex, with no other corner
 * in it.
 **/
bool ear(const std::vector<glm::vec2> & plane, const std::vector<std::size_t> & remaining, std::size_t at) {
    const std::size_t count = remaining.size();
    const glm::vec2 & p0 = plane[remaining[(at + count - 1) % count]];
    const glm::vec2 & p1 = plane[remaining[at]];
    const glm::vec2 & p2 = plane[remaining[(at + 1) % count]];
    if (turn(p0, p1, p2) <= 0.0F) {
        return false;
    }
    for (std::size_t other = 0; other < count; ++other) {
        const glm::vec2 & point = plane[remaining[other]];
        if (other == at || other == (at + 1) % count || other == (at + count - 1) % count) {
            continue;
        }
        // a corner at the same place as one of the ear's own is not inside it
        if (point == p0 || point == p1 || point == p2) {
            continue;
        }
        if (inside(point, p0, p1, p2)) {
            return false;
        }
    }
    return true;
}

/** Cuts one face of at least four corners, adding each triangle's corners and face to out. **/
void clip(const std::vector<glm::vec2> & plane, std::uint32_t first, std::uint32_t face, Triangles & out) {
    std::vector<std::size_t> remaining(plane.size());
    for (std::size_t i = 0; i < remaining.size(); ++i) {
        remaining[i] = i;
    }
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - a triangle's three corners, in order
    const auto emit = [&](std::size_t p0, std::size_t p1, std::size_t p2) {
        out.corners.push_back(first + static_cast<std::uint32_t>(p0));
        out.corners.push_back(first + static_cast<std::uint32_t>(p1));
        out.corners.push_back(first + static_cast<std::uint32_t>(p2));
        out.faces.push_back(face);
    };
    while (remaining.size() > 3) {
        const std::size_t count = remaining.size();
        bool cut = false;
        // a corner in line with its neighbours adds no area, and is left out before any ear is
        // cut, so no triangle is a sliver along a straight edge
        for (std::size_t at = 0; at < count && !cut; ++at) {
            const std::size_t before = remaining[(at + count - 1) % count];
            const std::size_t after = remaining[(at + 1) % count];
            if (turn(plane[before], plane[remaining[at]], plane[after]) == 0.0F) {
                remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(at));
                cut = true;
            }
        }
        for (std::size_t at = 0; at < count && !cut; ++at) {
            if (ear(plane, remaining, at)) {
                emit(remaining[(at + count - 1) % count], remaining[at], remaining[(at + 1) % count]);
                remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(at));
                cut = true;
            }
        }
        if (!cut) {
            // the edges cross, and no corner is an ear: what remains is cut as a fan
            for (std::size_t i = 1; i + 1 < remaining.size(); ++i) {
                emit(remaining[0], remaining[i], remaining[i + 1]);
            }
            return;
        }
    }
    if (turn(plane[remaining[0]], plane[remaining[1]], plane[remaining[2]]) != 0.0F) {
        emit(remaining[0], remaining[1], remaining[2]);
    }
}

};  // namespace

Triangles triangulate(const std::vector<glm::vec3> & points, const std::vector<std::int32_t> & vertices,
    const std::vector<std::int32_t> & indices) {
    Triangles out;
    std::size_t first = 0;
    for (std::size_t face = 0; face < vertices.size(); ++face) {
        if (vertices[face] < 0) {
            throw std::invalid_argument("face " + std::to_string(face) + " has a negative count of vertices");
        }
        const auto count = static_cast<std::size_t>(vertices[face]);
        if (first + count > indices.size()) {
            throw std::invalid_argument("the faces' vertices count more than the " + std::to_string(indices.size()) + " indices");
        }
        std::vector<glm::vec3> corners;
        corners.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            const std::int32_t index = indices[first + i];
            if (index < 0 || static_cast<std::size_t>(index) >= points.size()) {
                throw std::invalid_argument("index " + std::to_string(index) + " lies outside the " + std::to_string(points.size()) + " points");
            }
            corners.push_back(points[static_cast<std::size_t>(index)]);
        }
        const glm::vec3 normal = count >= 3 ? newell(corners) : glm::vec3(0.0F);
        if (normal == glm::vec3(0.0F)) {
            ++out.dropped;
        } else if (count == 3) {
            for (std::size_t i = 0; i < 3; ++i) {
                out.corners.push_back(static_cast<std::uint32_t>(first + i));
            }
            out.faces.push_back(static_cast<std::uint32_t>(face));
        } else {
            const std::size_t before = out.faces.size();
            clip(projected(corners, normal), static_cast<std::uint32_t>(first), static_cast<std::uint32_t>(face), out);
            if (out.faces.size() == before) {
                ++out.dropped;
            }
        }
        first += count;
    }
    if (first != indices.size()) {
        throw std::invalid_argument("the faces' vertices count " + std::to_string(first) + " of the " + std::to_string(indices.size()) + " indices");
    }
    return out;
}

};  // namespace v3d::moya::geometry
