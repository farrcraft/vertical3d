/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Polygon.h"

#include <cmath>
#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

#include "Plane.h"
#include "RenderContext.h"

#include "../../api/type/3dtypes.h"

namespace v3d::moya {

    namespace {

        /**
         * A piece is worth handing back only if it bounds something and is strictly smaller
         * than what it came from on some axis. A split that does not shrink its input would be
         * measured as undiceable again and split again, without end.
         **/
        bool progress(const boost::shared_ptr<Polygon> & piece, const v3d::type::AABBox & parent) {
            if (piece->vertexCount() < 3) {
                return false;
            }
            glm::vec3 was = parent.max() - parent.min();
            v3d::type::AABBox bound = piece->bound();
            glm::vec3 is = bound.max() - bound.min();
            return is[0] < was[0] || is[1] < was[1] || is[2] < was[2];
        }

    };  // namespace

    Polygon::Polygon() {
    }

    Polygon::~Polygon() {
    }

    void Polygon::addVertex(Vertex vert) {
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

    // return an object space bound of the polygon
    v3d::type::AABBox Polygon::bound(void) const {
        v3d::type::AABBox bound;

        if (vertices_.size() == 0) {
            return bound;
        }
        /*
            move over each vertex and record min/max
        */
        std::vector<Vertex>::const_iterator it = vertices_.begin();
        glm::vec3 min, max;
        min = vertices_[0].point();
        max = min;
        for (; it != vertices_.end(); it++) {
            // std::cerr << " v = " << (*it).point();
            if (min[0] > (*it).point()[0]) {
                min[0] = (*it).point()[0];
            }
            if (max[0] < (*it).point()[0]) {
                max[0] = (*it).point()[0];
            }

            if (min[1] > (*it).point()[1]) {
                min[1] = (*it).point()[1];
            }
            if (max[1] < (*it).point()[1]) {
                max[1] = (*it).point()[1];
            }

            if (min[2] > (*it).point()[2]) {
                min[2] = (*it).point()[2];
            }
            if (max[2] < (*it).point()[2]) {
                max[2] = (*it).point()[2];
            }
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
    void Polygon::split(const Plane& plane, const boost::shared_ptr<Polygon> & p1, const boost::shared_ptr<Polygon> & p2) {
        // intersect each edge with the plane
        glm::vec3 A, B, hit;
        int side;
        Vertex vert;
        size_t vcount;
        for (unsigned int i = 0; i < vertices_.size(); i++) {
            vcount = vertices_.size();
            A = vertices_[i].point();
            if (i == (vcount - 1)) {
                B = vertices_[0].point();
            } else {
                B = vertices_[i + 1].point();
            }
            // classify which side of the plane A is on
            side = plane.classify(A);
            if (plane.intersectEdge(A, B, &hit)) {
                // A is on one side and B on the other
                // hit is the common vertex both new edges share
                // create a new edge for p1 and p2 e1 (A, hit), e2 (hit, B)
                vert.point(A);
                if (i == 0) {
                    if (side < 0) {
                        p1->addVertex(vert);
                    } else {
                        p2->addVertex(vert);
                    }
                }

                vert.point(hit);
                p1->addVertex(vert);
                p2->addVertex(vert);

                vert.point(B);
                if (i != (vcount - 1)) {
                    if (side < 0) {
                        p2->addVertex(vert);
                    } else {
                        p1->addVertex(vert);
                    }
                }
            } else {
                // add edge to poly corresponding to the classification of A
                // since there was no intersection, B will be on the same side
                vert.point(A);
                int bside = plane.classify(B);
                assert(side == bside);
                if (side <= 0) {
                    if (i == 0) {
                        p1->addVertex(vert);
                    }
                    vert.point(B);
                    if (i != (vcount - 1)) {
                        p1->addVertex(vert);
                    }
                } else {
                    if (i == 0) {
                        p2->addVertex(vert);
                    }
                    vert.point(B);
                    if (i != (vcount - 1)) {
                        p2->addVertex(vert);
                    }
                }
            }
        }
    }

    void Polygon::split(RenderContext & rc) {
        // the cutting plane is derived from the first three vertices, and a polygon holding
        // fewer than that has no area to divide
        if (vertices_.size() < 3) {
            return;
        }

        // calculate polygon's normal from the polygon's first two vertices
        glm::vec3 v0, v1, n;
        v0 = vertices_[0].point() - vertices_[1].point();
        v1 = vertices_[2].point() - vertices_[1].point();
        n = glm::normalize(glm::cross(v1, v0));

        // calculate plane's normal
        glm::vec3 pn;
        pn = glm::normalize(glm::cross(n, v0));
        // find a point on the plane
        v3d::type::AABBox bounds;
        glm::vec3 mp, pop;
        bounds = bound();
        mp = (bounds.max() - bounds.min());
        mp /= 2.0;
        pop = bounds.min() + mp;
        // create the intersection plane from pop and pn
        Plane plane;
        plane.calculate(pn, pop);

        // two new (potentially) polygons created as a result of splitting
        boost::shared_ptr<Polygon> p1(new Polygon()), p2(new Polygon());
        // first split
        split(plane, p1, p2);

        // calculate normal for 2nd plane to split against
        pn = glm::normalize(glm::cross(n, pn));

        // create the new intersection plane
        plane.calculate(pn, pop);

        // split the two new polygons against the two created by the last split
        boost::shared_ptr<Polygon> pf1(new Polygon()),
            pf2(new Polygon()),
            pf3(new Polygon()),
            pf4(new Polygon());
        p1->split(plane, pf1, pf2);
        p2->split(plane, pf3, pf4);

        // the four pieces go back to the top of the renderer, which re-bounds, re-culls and
        // re-measures each against the grid; this polygon is discarded by the caller
        const boost::shared_ptr<Polygon> pieces[] = { pf1, pf2, pf3, pf4 };
        for (const boost::shared_ptr<Polygon> & piece : pieces) {
            if (progress(piece, bounds)) {
                // a piece is measured with the state its parent was submitted under, not with
                // whatever the current transformation and colour have since become
                if (placed()) {
                    piece->place(placement(), color());
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
            rest dropped, which is a wrong grid for a concave one and is what triangulating
            before dicing would fix.
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
                grid->addVertex(vert, i, j);
            }
        }

        diced_ = true;
        return true;
    }

};  // namespace v3d::moya
