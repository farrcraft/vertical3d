/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Polygon.h"

#include <api/type/geometry/Frustum.h>
#include <api/type/geometry/Plane.h>

#include <cmath>
#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include "RenderContext.h"

namespace v3d::moya {

namespace {

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
    v3d::type::geometry::AABBox bound = piece->bound();
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
    vertices_.erase(vertices_.begin() + idx);
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
        if (area > 1.0e-8f) {
            return across / area;
        }
    }
    return glm::vec3(0.0f);
}

/* note - clipping happens after culling

    this is a 3D Sutherland-Hodgman polygon clipper: instead of clipping against a single
    clipping rectangle edge, it clips against a plane.
*/
void Polygon::clip(const v3d::type::geometry::Plane & plane) {
    const size_t nverts = vertices_.size();
    // fewer than three vertices bound no area to keep, and the loop below starts on the
    // vertex before the first one
    if (nverts < 3) {
        return;
    }
    std::vector<Vertex> clipped;
    Vertex i;
    glm::vec3 hit;
    Vertex s = vertices_[nverts - 1];  // start with last vertex
    for (size_t j = 0; j < nverts; j++) {
        const Vertex p = vertices_[j];
        /*
         there are 4 possible test cases:
            case 1: s & p both inside	 - in/in
            case 2: s inside, p outside - in/out
            case 3: s & p both outside	 - out/out
            case 4: s outside, p inside - out/in
         */
        const bool pInside = plane.classify(p.point()) != v3d::type::geometry::Plane::NEGATIVE;
        const bool sInside = plane.classify(s.point()) != v3d::type::geometry::Plane::NEGATIVE;
        if (pInside) {  // cases 1 & 4
            if (!sInside) {  // case 4
                plane.intersectEdge(s.point(), p.point(), &hit);
                i.point(hit);
                clipped.push_back(i);
            }
            clipped.push_back(p);
        } else if (sInside) {  // case 2
            plane.intersectEdge(s.point(), p.point(), &hit);
            i.point(hit);
            clipped.push_back(i);
        }
        // case 3: the entire edge is clipped
        s = p;
    }

    vertices_ = std::move(clipped);
}

// return an object space bound of the polygon
v3d::type::geometry::AABBox Polygon::bound(void) const {
    v3d::type::geometry::AABBox bound;

    if (vertices_.empty()) {
        return bound;
    }
    /*
        move over each vertex and record min/max
    */
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

/* split the polygon into smaller polygons
   how do we decide where to split polygons?
   how many smaller polygons do we make?

   recursive subdivision seems like the way to do it.
   it would work similar to frustum clipping.
   a polygon in plane A will be divided into two halves by a plane B
   that is perpendicular to plane A.

   the generated sub polygons should be fed back to the active render context.

   each call to split should divide the polygon into 4 smaller pieces.

    there are two ways to go on this:
        - splitting using polygon/plane intersection
        - polygon triangulation

    the problem with triangulation is that the next step is dicing where we'll
    want to convert the triangles into a grid of equilateral micropolygons.

    using plane intersection suffers from not having a specific plane to use for
    the intersection. there's also no guarantee that the resulting geometry will
    be any better than what we'd get with triangulation.

    first, we'll try implementing plane intersection (since it should be easier
    than doing triangulation here). there are two ways to figure out what plane
    to use:
        - (poly.v1 - poly.v0).normalize().cross(poly.normal)
        - find rotation that maps poly.normal to [0.0, 0.0, 1.0]

    the first method uses an edge of the polygon and its normal to find a normal
    for the plane that is perpendicular to both.

    the second finds a matrix that can transform the polygon's vertices onto the
    x/y plane.

    the last piece we need is a point that lies on the plane. this should be the
    center of the polygon.
*/

/*
internal splitter - will be called 3 times by split()
args p1 and p2 should be pointers to empty polygons to store the resulting split
polygons
*/
namespace {

/**
 * A corner of a piece: its position, and the texture coordinates of the vertex it came
 * from. Nothing else is carried, since a piece takes its colour and normal from the state
 * its parent was submitted under.
 **/
Vertex carried(const Vertex & from) {
    Vertex vert;
    vert.point(from.point());
    if (from.hasTexCoord()) {
        vert.st(from.st());
    }
    return vert;
}

/**
 * Where an edge meets the plane, with the texture coordinates as far along the edge as the
 * point is.
 **/
Vertex crossing(const Vertex & a, const Vertex & b, const glm::vec3 & hit) {
    Vertex vert;
    vert.point(hit);
    if (a.hasTexCoord() && b.hasTexCoord()) {
        const float span = glm::length(b.point() - a.point());
        const float along = span > 0.0f ? glm::length(hit - a.point()) / span : 0.0f;
        vert.st(a.st() + (b.st() - a.st()) * along);
    }
    return vert;
}

/**
 * An edge that crosses the plane. hit is the vertex both halves come to share, so it goes
 * into each of them; which half keeps A and which keeps B follows the side A is on.
 **/
void addCrossingEdge(const Vertex& a, const Vertex& b, const glm::vec3& hit, int side,
    bool first, bool last, const boost::shared_ptr<Polygon>& p1, const boost::shared_ptr<Polygon>& p2) {
    if (first) {
        if (side < 0) {
            p1->addVertex(carried(a));
        } else {
            p2->addVertex(carried(a));
        }
    }

    const Vertex shared = crossing(a, b, hit);
    p1->addVertex(shared);
    p2->addVertex(shared);

    if (!last) {
        if (side < 0) {
            p2->addVertex(carried(b));
        } else {
            p1->addVertex(carried(b));
        }
    }
}

/**
 * An edge wholly on one side of the plane, so both its vertices go to the same half.
 *
 * Only the first edge contributes its A and only a non-final edge contributes its B: every
 * other vertex is the B of the edge before it.
 **/
void addWholeEdge(const Vertex& a, const Vertex& b, int side, bool first, bool last,
    const boost::shared_ptr<Polygon>& p1, const boost::shared_ptr<Polygon>& p2) {
    const boost::shared_ptr<Polygon>& half = side <= 0 ? p1 : p2;
    if (first) {
        half->addVertex(carried(a));
    }
    if (!last) {
        half->addVertex(carried(b));
    }
}

};  // namespace

void Polygon::split(const v3d::type::geometry::Plane& plane, const boost::shared_ptr<Polygon> & p1, const boost::shared_ptr<Polygon> & p2) {
    // intersect each edge with the plane
    glm::vec3 hit;
    for (unsigned int i = 0; i < vertices_.size(); i++) {
        const size_t vcount = vertices_.size();
        const Vertex & a = vertices_[i];
        const Vertex & b = i == (vcount - 1) ? vertices_[0] : vertices_[i + 1];
        // classify which side of the plane A is on
        const int side = plane.classify(a.point());
        const bool first = (i == 0);
        const bool last = (i == (vcount - 1));
        if (plane.intersectEdge(a.point(), b.point(), &hit)) {
            addCrossingEdge(a, b, hit, side, first, last, p1, p2);
        } else {
            // since there was no intersection, B will be on the same side
            assert(side == plane.classify(b.point()));
            addWholeEdge(a, b, side, first, last, p1, p2);
        }
    }
}

void Polygon::split(RenderContext & rc) {
    // the cutting plane is derived from the first three vertices, and a polygon holding
    // fewer than that has no area to divide
    if (vertices_.size() < 3) {
        return;
    }

    // the plane of the polygon, and an edge in it: the cutting plane is perpendicular to
    // both. A polygon with no plane - every vertex on one line - has no area to divide
    const glm::vec3 n = geometricNormal();
    if (n == glm::vec3(0.0f)) {
        return;
    }
    const glm::vec3 v0 = vertices_[0].point() - vertices_[1].point();

    // calculate plane's normal
    glm::vec3 pn;
    pn = glm::normalize(glm::cross(n, v0));
    // find a point on the plane
    v3d::type::geometry::AABBox bounds;
    glm::vec3 mp;
    glm::vec3 pop;
    bounds = bound();
    mp = (bounds.max() - bounds.min());
    mp /= 2.0;
    pop = bounds.min() + mp;
    // create the intersection plane from pop and pn
    v3d::type::geometry::Plane plane;
    plane.calculate(pn, pop);

    // two new (potentially) polygons created as a result of splitting
    boost::shared_ptr<Polygon> p1(new Polygon());
    boost::shared_ptr<Polygon> p2(new Polygon());
    // first split
    split(plane, p1, p2);

    // calculate normal for 2nd plane to split against
    pn = glm::normalize(glm::cross(n, pn));

    // create the new intersection plane
    plane.calculate(pn, pop);

    // split the two new polygons against the two created by the last split
    boost::shared_ptr<Polygon> pf1(new Polygon());
    boost::shared_ptr<Polygon> pf2(new Polygon());
    boost::shared_ptr<Polygon> pf3(new Polygon());
    boost::shared_ptr<Polygon> pf4(new Polygon());
    p1->split(plane, pf1, pf2);
    p2->split(plane, pf3, pf4);

    // the four pieces go back to the top of the renderer, which re-bounds, re-culls and
    // re-measures each against the grid; this polygon is discarded by the caller
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
    dice - turn the polygon into a grid of micropolygons
    dicing is done in eye space, so the grid is built in x, y, z and projected only when
    the hider needs raster coordinates
*/
bool Polygon::dice(boost::shared_ptr<MicroPolygonGrid> & grid, RenderContext & rc) {
    // one grid covers the whole polygon at this sampling, so the call after the first is
    // the end of the sequence rather than another grid
    if (diced_ || vertices_.size() < 3) {
        return false;
    }

    /*
        the grid size is the number of micropolygons in a grid, and a grid of n vertices a
        side is n - 1 of them a side. The first pass has already split anything whose
        raster bound is larger than that, so one grid is enough for what reaches here.
    */
    unsigned int across = static_cast<unsigned int>(sqrt(static_cast<float>(rc.gridSize())));
    grid.reset(new MicroPolygonGrid(across + 1));

    /*
        bilinear interpolation over the polygon's first four vertices. A triangle's fourth
        corner degenerates onto its third; a polygon with more than four vertices has the
        rest dropped, which gives a wrong grid for a concave one. Triangulating before
        dicing would avoid that.
    */
    const size_t fourth = vertices_.size() > 3 ? 3 : 2;
    glm::vec3 corners[4] = {
        vertices_[0].point(),
        vertices_[1].point(),
        vertices_[2].point(),
        vertices_[fourth].point()
    };
    // colour interpolates the same way the position does, so a "Cs" given per vertex
    // reaches the grid. It is the geometry's colour rather than a shaded one
    glm::vec3 colors[4] = {
        vertices_[0].color(),
        vertices_[1].color(),
        vertices_[2].color(),
        vertices_[fourth].color()
    };
    // and so does the shading normal, so a surface given a varying "N" comes out smooth
    // rather than faceted
    glm::vec3 normals[4] = {
        vertices_[0].normal(),
        vertices_[1].normal(),
        vertices_[2].normal(),
        vertices_[fourth].normal()
    };
    // the geometric normal is one value across the primitive, so there is nothing to
    // interpolate: addPolygon() wrote the same one onto every vertex
    const glm::vec3 geometric = vertices_[0].geometricNormal();
    // texture coordinates interpolate only when the scene gave every corner one; a grid
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
        float u = static_cast<float>(i) / span;
        for (unsigned int j = 0; j < size; j++) {
            float w = static_cast<float>(j) / span;
            Vertex vert;
            vert.point(corners[0] * ((1.0f - u) * (1.0f - w)) +
                       corners[1] * (u * (1.0f - w)) +
                       corners[2] * (u * w) +
                       corners[3] * ((1.0f - u) * w));
            vert.color(colors[0] * ((1.0f - u) * (1.0f - w)) +
                       colors[1] * (u * (1.0f - w)) +
                       colors[2] * (u * w) +
                       colors[3] * ((1.0f - u) * w));
            const glm::vec3 normal = normals[0] * ((1.0f - u) * (1.0f - w)) +
                                     normals[1] * (u * (1.0f - w)) +
                                     normals[2] * (u * w) +
                                     normals[3] * ((1.0f - u) * w);
            // interpolating unit normals does not give a unit one back
            const float length = glm::length(normal);
            vert.normal(length > 0.0f ? normal / length : geometric);
            vert.geometricNormal(geometric);
            if (textured) {
                vert.st(st[0] * ((1.0f - u) * (1.0f - w)) +
                        st[1] * (u * (1.0f - w)) +
                        st[2] * (u * w) +
                        st[3] * ((1.0f - u) * w));
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
