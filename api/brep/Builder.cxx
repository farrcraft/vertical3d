/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Builder.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "BRep.h"
#include "HalfEdge.h"
#include "Index.h"
#include "Topology.h"

#include <boost/make_shared.hpp>
#include <glm/geometric.hpp>

namespace v3d::brep {

namespace {

/**
 * A position's exact bits, for finding every point at it in one lookup.
 **/
using PositionKey = std::array<std::uint32_t, 3>;

/**
 * A float holding a small whole number has its low bits clear, and a table picks a bucket by
 * the low bits of the hash, so the words are folded together and handed to the standard hash,
 * which mixes every bit.
 **/
struct PositionHash {
    std::size_t operator()(const PositionKey& key) const noexcept {
        const std::uint64_t folded = (static_cast<std::uint64_t>(key[0]) << 32U) ^ (static_cast<std::uint64_t>(key[1]) << 16U) ^ key[2];
        return std::hash<std::uint64_t>{}(folded);
    }
};

PositionKey positionKey(const glm::vec3& point) {
    // adding zero turns -0 into 0, which compares equal to it but has other bits
    return { std::bit_cast<std::uint32_t>(point.x + 0.0F), std::bit_cast<std::uint32_t>(point.y + 0.0F),
        std::bit_cast<std::uint32_t>(point.z + 0.0F) };
}

/**
 * An edge in one direction, from one vertex to another.
 **/
std::uint64_t edgeKey(Index from, Index to) {
    return (static_cast<std::uint64_t>(from) << 32U) | to;
}

/**
 * Two triangles that could be joined into a quad across the edge they share.
 **/
struct Candidate {
    float angle;
    std::size_t first;
    std::size_t second;
    Index from;
    Index to;
};

/**
 * A triangle's corners turned so that the one after `from` is `to`.
 **/
std::array<Index, 3> startingAt(const std::vector<Index>& triangle, Index from) {
    std::size_t start = 0;
    while (triangle[start] != from) {
        start++;
    }
    return { triangle[start], triangle[(start + 1) % 3], triangle[(start + 2) % 3] };
}

float squared(const glm::vec3& vector) {
    return glm::dot(vector, vector);
}

/**
 * Whether the edge from `from` to `to` is at least as long as each of the triangle's other two.
 **/
bool longestEdge(const std::vector<glm::vec3>& positions, const std::array<Index, 3>& corners) {
    const float shared = squared(positions[corners[1]] - positions[corners[0]]);
    return shared >= squared(positions[corners[2]] - positions[corners[1]]) &&
        shared >= squared(positions[corners[0]] - positions[corners[2]]);
}

/**
 * Whether every corner of a quad turns the same way about a normal.
 **/
bool convex(const std::vector<glm::vec3>& positions, const std::array<Index, 4>& quad, const glm::vec3& normal) {
    int turns = 0;
    for (std::size_t corner = 0; corner < 4; corner++) {
        const glm::vec3& before = positions[quad[(corner + 3) % 4]];
        const glm::vec3& at = positions[quad[corner]];
        const glm::vec3& after = positions[quad[(corner + 1) % 4]];
        const float turn = glm::dot(glm::cross(at - before, after - at), normal);
        if (turn == 0.0F) {
            return false;
        }
        turns += turn > 0.0F ? 1 : -1;
    }
    return turns == 4 || turns == -4;
}

/**
 * Join pairs of triangles back into quads, in place. A joined pair leaves the quad in the first
 * triangle's place and an empty loop in the second's.
 **/
void joinTriangles(const std::vector<glm::vec3>& positions, std::vector<std::vector<Index>>* loops, float maximum) {
    // an edge two triangles run along in the same direction is left as it is, for build() to
    // refuse, rather than joined away
    std::unordered_map<std::uint64_t, std::size_t> triangleOf;
    std::unordered_set<std::uint64_t> crowded;
    std::vector<glm::vec3> normals(loops->size(), glm::vec3(0.0F));
    for (std::size_t loop = 0; loop < loops->size(); loop++) {
        const std::vector<Index>& corners = (*loops)[loop];
        if (corners.size() != 3) {
            continue;
        }
        const glm::vec3 normal = glm::cross(positions[corners[1]] - positions[corners[0]],
            positions[corners[2]] - positions[corners[0]]);
        const float length = glm::length(normal);
        if (length <= 0.0F) {
            continue;
        }
        normals[loop] = normal / length;
        for (std::size_t corner = 0; corner < 3; corner++) {
            const std::uint64_t key = edgeKey(corners[corner], corners[(corner + 1) % 3]);
            if (!triangleOf.emplace(key, loop).second) {
                crowded.insert(key);
            }
        }
    }

    std::vector<Candidate> candidates;
    for (const auto& [key, first] : triangleOf) {
        const Index from = static_cast<Index>(key >> 32U);
        const Index to = static_cast<Index>(key & 0xFFFFFFFFULL);
        const auto across = triangleOf.find(edgeKey(to, from));
        if (across == triangleOf.end() || across->second <= first || crowded.contains(key) ||
            crowded.contains(across->first)) {
            continue;
        }
        const std::size_t second = across->second;
        const float angle = std::acos(std::clamp(glm::dot(normals[first], normals[second]), -1.0F, 1.0F));
        if (angle > maximum) {
            continue;
        }
        const std::array<Index, 3> one = startingAt((*loops)[first], from);
        const std::array<Index, 3> other = startingAt((*loops)[second], to);
        if (!longestEdge(positions, one) || !longestEdge(positions, other)) {
            continue;
        }
        const std::array<Index, 4> quad{ one[1], one[2], one[0], other[2] };
        if (!convex(positions, quad, normals[first] + normals[second])) {
            continue;
        }
        candidates.push_back({ angle, first, second, from, to });
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
        return std::tie(left.angle, left.first, left.second) < std::tie(right.angle, right.first, right.second);
    });

    std::vector<bool> joined(loops->size(), false);
    for (const Candidate& candidate : candidates) {
        if (joined[candidate.first] || joined[candidate.second]) {
            continue;
        }
        joined[candidate.first] = true;
        joined[candidate.second] = true;
        // the first runs from, to, c and the second to, from, q. The quad drops the edge between
        // from and to and keeps the other four in loop order
        const std::array<Index, 3> one = startingAt((*loops)[candidate.first], candidate.from);
        const std::array<Index, 3> other = startingAt((*loops)[candidate.second], candidate.to);
        (*loops)[candidate.first] = { one[1], one[2], one[0], other[2] };
        (*loops)[candidate.second].clear();
    }
}

/**
 * The faces as loops of welded vertices, each clockwise seen from outside.
 **/
struct Faces {
    std::vector<glm::vec3> positions;
    std::vector<std::vector<Index>> loops;
    std::size_t corners = 0;
    std::size_t skipped = 0;
};

/**
 * Why the counts and the points cannot be read, or nothing when they can.
 **/
std::string checkInput(const Polygons& polygons) {
    std::size_t total = 0;
    for (const std::uint32_t count : polygons.counts) {
        total += count;
    }
    if (total != polygons.indices.size()) {
        return "the faces have " + std::to_string(total) + " corners between them, and there are " +
            std::to_string(polygons.indices.size()) + " indices";
    }
    for (std::size_t point = 0; point < polygons.points.size(); point++) {
        const glm::vec3& position = polygons.points[point];
        if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z)) {
            return "point " + std::to_string(point) + " is not finite";
        }
    }
    return {};
}

/**
 * Gives each position one vertex. Only the points a face uses become vertices.
 **/
class Welder final {
 public:
    Welder(const Polygons& polygons, std::vector<glm::vec3>* positions)
        : polygons_(polygons), positions_(positions), vertexOf_(polygons.points.size(), INVALID_ID) {
        vertexAt_.reserve(polygons.points.size());
    }

    Index vertex(std::uint32_t point) {
        if (vertexOf_[point] == INVALID_ID) {
            const auto [found, added] = vertexAt_.emplace(positionKey(polygons_.points[point]),
                static_cast<Index>(positions_->size()));
            if (added) {
                positions_->push_back(polygons_.points[point]);
            }
            vertexOf_[point] = found->second;
        }
        return vertexOf_[point];
    }

 private:
    const Polygons& polygons_;
    std::vector<glm::vec3>* positions_;
    std::vector<Index> vertexOf_;
    std::unordered_map<PositionKey, Index, PositionHash> vertexAt_;
};

/**
 * Read every face into a loop of vertices. A face with fewer than three distinct corners is
 * counted and left out.
 *
 * @return why a face cannot be read, or nothing when every face can
 **/
std::string readFaces(const Polygons& polygons, Winding winding, Faces* faces) {
    Welder welder(polygons, &faces->positions);
    faces->loops.reserve(polygons.counts.size());
    std::size_t next = 0;
    for (std::size_t face = 0; face < polygons.counts.size(); face++) {
        std::vector<Index> loop;
        for (std::uint32_t corner = 0; corner < polygons.counts[face]; corner++) {
            const std::uint32_t point = polygons.indices[next++];
            if (point >= polygons.points.size()) {
                return "face " + std::to_string(face) + " names point " + std::to_string(point) + ", and there are " +
                    std::to_string(polygons.points.size());
            }
            const Index vertex = welder.vertex(point);
            if (loop.empty() || loop.back() != vertex) {
                loop.push_back(vertex);
            }
        }
        while (loop.size() > 1 && loop.back() == loop.front()) {
            loop.pop_back();
        }
        if (loop.size() < 3) {
            faces->skipped++;
            continue;
        }
        std::vector<Index> sorted = loop;
        std::sort(sorted.begin(), sorted.end());
        if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end()) {
            return "face " + std::to_string(face) + " visits one vertex twice";
        }
        if (winding == Winding::CounterClockwise) {
            std::reverse(loop.begin(), loop.end());
        }
        faces->corners += loop.size();
        faces->loops.push_back(std::move(loop));
    }
    return {};
}

/**
 * Make the mesh's half edges and faces, and pair each half edge with the one that runs the
 * other way between the same two vertices. A half edge ends at its corner and starts at the
 * corner before it.
 *
 * @return why two faces cannot share an edge, or nothing when every edge is sound
 **/
std::string linkFaces(const Faces& faces, BRep::Snapshot* parts) {
    parts->vertices.reserve(faces.positions.size());
    for (const glm::vec3& position : faces.positions) {
        parts->vertices.emplace_back(position);
    }
    parts->faces.reserve(faces.loops.size());
    parts->edges.reserve(faces.corners);

    std::unordered_map<std::uint64_t, Index> edgeFrom;
    edgeFrom.reserve(faces.corners);
    std::vector<glm::vec3> corners;
    for (const std::vector<Index>& loop : faces.loops) {
        const Index face = static_cast<Index>(parts->faces.size());
        const Index first = static_cast<Index>(parts->edges.size());
        corners.clear();
        for (std::size_t corner = 0; corner < loop.size(); corner++) {
            const Index from = loop[(corner + loop.size() - 1) % loop.size()];
            const Index to = loop[corner];
            const Index id = static_cast<Index>(parts->edges.size());
            if (!edgeFrom.emplace(edgeKey(from, to), id).second) {
                return "two faces run from vertex " + std::to_string(from) + " to vertex " + std::to_string(to) +
                    ": more than two faces meet there, or the faces disagree about which side is outside";
            }
            HalfEdge half(to);
            half.face(face);
            half.next(first + static_cast<Index>((corner + 1) % loop.size()));
            const auto across = edgeFrom.find(edgeKey(to, from));
            if (across != edgeFrom.end()) {
                half.pair(across->second);
                parts->edges[across->second].pair(id);
            }
            parts->edges.push_back(half);
            corners.push_back(faces.positions[to]);
        }
        parts->faces.emplace_back(newellNormal(corners), first);
    }
    return {};
}

};  // namespace

Built build(const Polygons& polygons, const BuildOptions& options) {
    Built built;
    built.problem = checkInput(polygons);
    if (!built.problem.empty()) {
        return built;
    }
    Faces faces;
    built.problem = readFaces(polygons, options.winding, &faces);
    if (!built.problem.empty()) {
        return built;
    }
    built.skipped = faces.skipped;
    if (options.joinTriangles) {
        joinTriangles(faces.positions, &faces.loops, options.joinAngle);
        faces.loops.erase(std::remove_if(faces.loops.begin(), faces.loops.end(),
            [](const std::vector<Index>& loop) { return loop.empty(); }), faces.loops.end());
    }
    BRep::Snapshot parts;
    built.problem = linkFaces(faces, &parts);
    if (!built.problem.empty()) {
        return built;
    }
    built.mesh = boost::make_shared<BRep>();
    built.mesh->restore(parts);
    return built;
}

};  // namespace v3d::brep
