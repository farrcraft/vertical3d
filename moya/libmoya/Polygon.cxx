/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Polygon.h"

#include <api/type/geometry/AABBox.h>
#include <api/type/geometry/Frustum.h>
#include <api/type/geometry/Plane.h>
#include <moya/libmoya/reyes/MicroPolygonGrid.h>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include "RenderContext.h"
#include "Vertex.h"

namespace v3d::moya {

namespace {

/**
 * The least length of the cross product of two edges from a polygon's first vertex that gives
 * the polygon a normal.
 **/
constexpr float SMALLEST_SPANNED_AREA = 1.0e-8F;

Vertex crossing(const Vertex & start, const Vertex & end, const glm::vec3 & hit);

/**
 * Where an edge whose ends classify on opposite sides of a plane meets it. The two tests round
 * differently, so intersectEdge can find no meeting for an edge that ends a hair past the
 * plane. The edge then meets the plane at the end nearer to it.
 **/
glm::vec3 meeting(const v3d::type::geometry::Plane & plane, const glm::vec3 & start, const glm::vec3 & end) {
    glm::vec3 hit;
    if (plane.intersectEdge(start, end, &hit)) {
        return hit;
    }
    return std::abs(plane.signedDistance(start)) <= std::abs(plane.signedDistance(end)) ? start : end;
}

/**
 * A piece is worth handing back only if it bounds something and is strictly smaller
 * than what it came from on some axis. A split that does not shrink its input would be
 * measured as undiceable again and split again, without end.
 **/
bool progress(const boost::shared_ptr<Polygon> & piece, const v3d::type::geometry::AABBox & parent) {
    if (piece->vertexCount() < 3) {
        return false;
    }
    glm::vec3 was = parent.max() - parent.min();
    const v3d::type::geometry::AABBox bound = piece->bound();
    glm::vec3 is = bound.max() - bound.min();
    return is[0] < was[0] || is[1] < was[1] || is[2] < was[2];
}

};  // namespace

Polygon::Polygon() {
}

Polygon::~Polygon() {
}

void Polygon::addVertex(const Vertex& vert) {
    vertices_.push_back(vert);
}

size_t Polygon::vertexCount(void) const {
    return vertices_.size();
}

Vertex Polygon::vertex(size_t idx) const {
    assert(idx < vertices_.size());
    return vertices_[idx];
}

void Polygon::removeVertex(size_t idx) {
    assert(idx < vertices_.size());
    vertices_.erase(vertices_.begin() + static_cast<std::ptrdiff_t>(idx));
}

void Polygon::clear(void) {
    vertices_.clear();
}

Vertex& Polygon::operator[] (size_t idx) {
    assert(idx < vertices_.size());
    return vertices_[idx];
}

glm::vec3 Polygon::geometricNormal(void) const {
    // the first pair of edges that spans an area. A repeated vertex or a collinear run at
    // the head of the polygon gives a zero cross product, which names no plane, so the
    // search continues past it
    for (size_t i = 1; i + 1 < vertices_.size(); i++) {
        const glm::vec3 across = glm::cross(vertices_[i].point() - vertices_[0].point(),
                                            vertices_[i + 1].point() - vertices_[0].point());
        const float area = glm::length(across);
        if (area > SMALLEST_SPANNED_AREA) {
            return across / area;
        }
    }
    return glm::vec3(0.0F);
}

void Polygon::clip(const v3d::type::geometry::Plane & plane) {
    const size_t nverts = vertices_.size();
    // fewer than three vertices bound no area to keep, and the loop below starts on the
    // vertex before the first one
    if (nverts < 3) {
        return;
    }
    std::vector<Vertex> clipped;
    Vertex s = vertices_[nverts - 1];  // start with last vertex
    for (size_t j = 0; j < nverts; j++) {
        const Vertex current = vertices_[j];
        /*
            s is the previous vertex.
            case 1: s and current both inside.
            case 2: s inside, current outside.
            case 3: s and current both outside.
            case 4: s outside, current inside.
        */
        const bool currentInside = plane.classify(current.point()) != v3d::type::geometry::Plane::NEGATIVE;
        const bool sInside = plane.classify(s.point()) != v3d::type::geometry::Plane::NEGATIVE;
        if (currentInside) {  // cases 1 & 4
            if (!sInside) {  // case 4
                clipped.push_back(crossing(s, current, meeting(plane, s.point(), current.point())));
            }
            clipped.push_back(current);
        } else if (sInside) {  // case 2
            clipped.push_back(crossing(s, current, meeting(plane, s.point(), current.point())));
        }
        // case 3: the entire edge is clipped
        s = current;
    }

    vertices_ = std::move(clipped);
}

// the bound in the space the vertices are in: object space, until RenderContext::addPolygon()
// moves a diceable polygon's vertices into eye space
v3d::type::geometry::AABBox Polygon::bound(void) const {
    v3d::type::geometry::AABBox bound;

    if (vertices_.empty()) {
        return bound;
    }
    std::vector<Vertex>::const_iterator it = vertices_.begin();
    glm::vec3 min;
    glm::vec3 max;
    min = vertices_[0].point();
    max = min;
    for (; it != vertices_.end(); it++) {
        min = glm::min(min, (*it).point());
        max = glm::max(max, (*it).point());
    }
    bound.extents(min, max);

    return bound;
}

namespace {

/**
 * A corner of a piece: the vertex it came from, with the position, colour, shading normal and
 * texture coordinates that vertex has. A value the vertex was not given stays unset, so the
 * piece fills it from the primitive's state as its parent was filled.
 **/
Vertex carried(const Vertex & from) {
    Vertex vert;
    vert.point(from.point());
    if (from.hasColor()) {
        vert.color(from.color());
    }
    if (from.hasNormal()) {
        vert.normal(from.normal());
    }
    if (from.hasTexCoord()) {
        vert.st(from.st());
    }
    return vert;
}

/**
 * The shading normal part of the way from one normal to another, as a unit vector. Two equal
 * normals give that normal back unchanged. Opposite normals meeting half way give zero, which
 * names no direction, so false is returned and the crossing is left without a normal of its own.
 **/
bool between(const glm::vec3 & from, const glm::vec3 & to, float along, glm::vec3* normal) {
    if (from == to) {
        *normal = from;
        return true;
    }
    // interpolating unit normals does not give a unit one back
    const glm::vec3 mixed = from + (to - from) * along;
    const float length = glm::length(mixed);
    if (!(length > 0.0F)) {
        return false;
    }
    *normal = mixed / length;
    return true;
}

/**
 * Where an edge meets the plane. Colour, shading normal and texture coordinates are taken as
 * far along the edge as the point is, each only when both ends of the edge have one. The
 * normal is renormalised.
 **/
Vertex crossing(const Vertex & start, const Vertex & end, const glm::vec3 & hit) {
    Vertex vert;
    vert.point(hit);
    const float span = glm::length(end.point() - start.point());
    const float along = span > 0.0F ? glm::length(hit - start.point()) / span : 0.0F;
    if (start.hasColor() && end.hasColor()) {
        vert.color(start.color() + (end.color() - start.color()) * along);
    }
    glm::vec3 normal(0.0F);
    if (start.hasNormal() && end.hasNormal() && between(start.normal(), end.normal(), along, &normal)) {
        vert.normal(normal);
    }
    if (start.hasTexCoord() && end.hasTexCoord()) {
        vert.st(start.st() + (end.st() - start.st()) * along);
    }
    return vert;
}

/**
 * An edge that crosses the plane. hit is the vertex both halves share, so it goes into each
 * of them. The side start is on decides which half keeps start and which keeps end.
 **/
void addCrossingEdge(const Vertex& start, const Vertex& end, const glm::vec3& hit, int side,
    bool first, bool last, const boost::shared_ptr<Polygon>& p1, const boost::shared_ptr<Polygon>& p2) {
    if (first) {
        if (side < 0) {
            p1->addVertex(carried(start));
        } else {
            p2->addVertex(carried(start));
        }
    }

    const Vertex shared = crossing(start, end, hit);
    p1->addVertex(shared);
    p2->addVertex(shared);

    if (!last) {
        if (side < 0) {
            p2->addVertex(carried(end));
        } else {
            p1->addVertex(carried(end));
        }
    }
}

/**
 * An edge wholly on one side of the plane, so both its vertices go to the same half.
 *
 * Only the first edge contributes its start and only a non-final edge contributes its end:
 * every other vertex is the end of the edge before it.
 **/
void addWholeEdge(const Vertex& start, const Vertex& end, int side, bool first, bool last,
    const boost::shared_ptr<Polygon>& p1, const boost::shared_ptr<Polygon>& p2) {
    const boost::shared_ptr<Polygon>& half = side <= 0 ? p1 : p2;
    if (first) {
        half->addVertex(carried(start));
    }
    if (!last) {
        half->addVertex(carried(end));
    }
}

};  // namespace

/*
    Cut the polygon by a plane. p1 takes the negative side and p2 the positive, and both
    start empty. split(RenderContext &) calls this three times to make four pieces.
*/
void Polygon::split(const v3d::type::geometry::Plane& plane, const boost::shared_ptr<Polygon> & p1, const boost::shared_ptr<Polygon> & p2) {
    // intersect each edge with the plane
    glm::vec3 hit;
    for (unsigned int i = 0; i < vertices_.size(); i++) {
        const size_t vcount = vertices_.size();
        const Vertex & start = vertices_[i];
        const Vertex & end = i == (vcount - 1) ? vertices_[0] : vertices_[i + 1];
        const int side = plane.classify(start.point());
        const bool first = (i == 0);
        const bool last = (i == (vcount - 1));
        if (plane.intersectEdge(start.point(), end.point(), &hit)) {
            addCrossingEdge(start, end, hit, side, first, last, p1, p2);
        } else if (side != plane.classify(end.point())) {
            // the two tests round differently, so an edge that ends a hair past the plane
            // still crosses it
            addCrossingEdge(start, end, meeting(plane, start.point(), end.point()), side, first, last, p1, p2);
        } else {
            addWholeEdge(start, end, side, first, last, p1, p2);
        }
    }
}

/*
    Cut the polygon into four by two planes through the centre of its bound. Both are
    perpendicular to the polygon's plane, and the second is perpendicular to the first.
*/
void Polygon::split(RenderContext & rc) {
    // a polygon with fewer than three vertices has no area to divide
    if (vertices_.size() < 3) {
        return;
    }

    // the plane of the polygon, and an edge in it: the normal of the first cutting plane is
    // perpendicular to both. A polygon with no plane, every vertex on one line, has no area
    const glm::vec3 surfaceNormal = geometricNormal();
    if (surfaceNormal == glm::vec3(0.0F)) {
        return;
    }
    // the edge is taken from the first vertex that differs from the head. A repeated head
    // gives a zero edge, which names no cutting plane. A polygon with a plane has such a vertex
    glm::vec3 v0(0.0F);
    for (size_t i = 1; i < vertices_.size() && v0 == glm::vec3(0.0F); i++) {
        v0 = vertices_[0].point() - vertices_[i].point();
    }

    // the first cutting plane is perpendicular to the polygon's plane and parallel to the edge
    glm::vec3 pn;
    pn = glm::normalize(glm::cross(surfaceNormal, v0));
    // both cutting planes pass through the centre of the bound
    v3d::type::geometry::AABBox bounds;
    glm::vec3 mp;
    glm::vec3 pop;
    bounds = bound();
    mp = (bounds.max() - bounds.min());
    mp /= 2.0;
    pop = bounds.min() + mp;
    v3d::type::geometry::Plane plane;
    plane.calculate(pn, pop);

    // the two halves of the first cut. A half that receives no vertices stays empty
    const boost::shared_ptr<Polygon> p1(new Polygon());
    const boost::shared_ptr<Polygon> p2(new Polygon());
    split(plane, p1, p2);

    // the second cutting plane is perpendicular to the polygon's plane and to the first
    pn = glm::normalize(glm::cross(surfaceNormal, pn));

    plane.calculate(pn, pop);

    // each half is cut again, into quarters
    const boost::shared_ptr<Polygon> pf1(new Polygon());
    const boost::shared_ptr<Polygon> pf2(new Polygon());
    const boost::shared_ptr<Polygon> pf3(new Polygon());
    const boost::shared_ptr<Polygon> pf4(new Polygon());
    p1->split(plane, pf1, pf2);
    p2->split(plane, pf3, pf4);

    // each piece goes back through RenderContext::addPolygon(), which bounds, culls and
    // measures it against the grid again. The caller discards this polygon
    const boost::shared_ptr<Polygon> pieces[] = { pf1, pf2, pf3, pf4 };
    for (const boost::shared_ptr<Polygon> & piece : pieces) {
        if (progress(piece, bounds)) {
            // a piece is measured with the state its parent was submitted under, not with
            // whatever the current transformation, colour and shader have since become
            if (placed()) {
                piece->place(placement(), color(), normal(), shading());
                piece->motion(motion());
            }
            rc.addPolygon(piece);
        }
    }
}

/*
    RenderContext::addPolygon() moves a diceable polygon's vertices into eye space, so the
    grid is built in eye space. The hider projects it to raster space.
*/
bool Polygon::dice(boost::shared_ptr<reyes::MicroPolygonGrid> & grid, RenderContext & rc) {
    // one grid covers the whole polygon, so the second call ends the sequence
    if (diced_ || vertices_.size() < 3) {
        return false;
    }

    /*
        The grid size is the number of micropolygons in a grid, and a grid of n vertices a
        side has n - 1 micropolygons a side. RenderContext::addPolygon() marks for splitting
        anything whose raster bound is wider or taller than the square root of the grid size
        times the shading rate, so one grid covers what reaches here.
    */
    const unsigned int across = static_cast<unsigned int>(sqrt(static_cast<float>(rc.gridSize())));
    grid.reset(new reyes::MicroPolygonGrid(across + 1));

    /*
        bilinear interpolation over the polygon's first four vertices. A triangle's fourth
        corner degenerates onto its third. A polygon with more than four vertices has the
        rest dropped, so its grid does not cover the part of the polygon past the fourth
        vertex.
    */
    const size_t fourth = vertices_.size() > 3 ? 3 : 2;
    const glm::vec3 corners[4] = {
        vertices_[0].point(),
        vertices_[1].point(),
        vertices_[2].point(),
        vertices_[fourth].point()
    };
    // colour interpolates the same way the position does, so a "Cs" given per vertex
    // reaches the grid. It is the geometry's colour rather than a shaded one
    const glm::vec3 colors[4] = {
        vertices_[0].color(),
        vertices_[1].color(),
        vertices_[2].color(),
        vertices_[fourth].color()
    };
    // the shading normal interpolates the same way, so a surface given a varying "N" is
    // shaded smooth rather than faceted
    const glm::vec3 normals[4] = {
        vertices_[0].normal(),
        vertices_[1].normal(),
        vertices_[2].normal(),
        vertices_[fourth].normal()
    };
    // the geometric normal is one value across the primitive, so there is nothing to
    // interpolate: addPolygon() wrote the same one onto every vertex
    const glm::vec3 geometric = vertices_[0].geometricNormal();
    // texture coordinates interpolate only when the scene gave every corner one. A grid
    // without them takes its own parameters as s and t when it is shaded
    const bool textured = vertices_[0].hasTexCoord() && vertices_[1].hasTexCoord() &&
        vertices_[2].hasTexCoord() && vertices_[fourth].hasTexCoord();
    const glm::vec2 st[4] = {
        vertices_[0].st(),
        vertices_[1].st(),
        vertices_[2].st(),
        vertices_[fourth].st()
    };

    const unsigned int size = grid->size();
    const float span = static_cast<float>(size - 1);
    for (unsigned int i = 0; i < size; i++) {
        const float u = static_cast<float>(i) / span;
        for (unsigned int j = 0; j < size; j++) {
            const float w = static_cast<float>(j) / span;
            Vertex vert;
            vert.point(corners[0] * ((1.0F - u) * (1.0F - w)) +
                       corners[1] * (u * (1.0F - w)) +
                       corners[2] * (u * w) +
                       corners[3] * ((1.0F - u) * w));
            vert.color(colors[0] * ((1.0F - u) * (1.0F - w)) +
                       colors[1] * (u * (1.0F - w)) +
                       colors[2] * (u * w) +
                       colors[3] * ((1.0F - u) * w));
            const glm::vec3 normal = normals[0] * ((1.0F - u) * (1.0F - w)) +
                                     normals[1] * (u * (1.0F - w)) +
                                     normals[2] * (u * w) +
                                     normals[3] * ((1.0F - u) * w);
            // interpolating unit normals does not give a unit one back
            const float length = glm::length(normal);
            vert.normal(length > 0.0F ? normal / length : geometric);
            vert.geometricNormal(geometric);
            if (textured) {
                vert.st(st[0] * ((1.0F - u) * (1.0F - w)) +
                        st[1] * (u * (1.0F - w)) +
                        st[2] * (u * w) +
                        st[3] * ((1.0F - u) * w));
            }
            grid->addVertex(vert, i, j);
        }
    }

    diced_ = true;
    return true;
}

void clip(Polygon & poly, const v3d::type::geometry::Frustum & frustum) {
    for (const v3d::type::geometry::Plane & plane : frustum.planes()) {
        poly.clip(plane);
    }
}

};  // namespace v3d::moya
