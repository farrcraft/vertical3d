/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <moya/libmoya/reyes/Primitive.h>

#include <vector>

#include "Vertex.h"

#include <boost/shared_ptr.hpp>

namespace v3d::type::geometry {
class Frustum;
class Plane;
};  // namespace v3d::type::geometry

namespace v3d::moya {
class RenderContext;

class Polygon : public reyes::Primitive {
 public:
    Polygon();
    virtual ~Polygon();

    /**
     **/
    void addVertex(const Vertex& vert);

    /**
     **/
    size_t vertexCount(void) const;

    /**
     **/
    Vertex vertex(size_t idx) const;

    /**
     **/
    void removeVertex(size_t idx);

    /**
     **/
    Vertex & operator[] (size_t idx);

    /**
     **/
    void clear(void);

    /**
     * The unit normal of the plane the polygon lies in, wound the way the vertices are. It
     * comes from the first vertex and the first pair of later neighbours not collinear with it.
     *
     * It is one value for the whole polygon, and is the shading language's Ng. A polygon
     * whose vertices are collinear, or which has fewer than three, has no plane and returns zero.
     **/
    glm::vec3 geometricNormal(void) const;

    /**
     * Sutherland-Hodgman clip of the polygon against a plane, keeping the positive half
     * space. The polygon is rewritten in place, so a caller can clip against one plane after
     * another.
     **/
    void clip(const v3d::type::geometry::Plane & plane);

    // reyes::Primitive
    virtual v3d::type::geometry::AABBox bound(void) const;
    virtual void split(RenderContext & rc);
    virtual bool dice(boost::shared_ptr<reyes::MicroPolygonGrid> & grid, RenderContext & rc);
    /**
     * The same, into a grid of `across` micropolygons a side rather than the context's grid
     * size. The vertices have to be in eye space already, as addPolygon() leaves them.
     **/
    bool dice(boost::shared_ptr<reyes::MicroPolygonGrid> & grid, unsigned int across);

 protected:
    void split(const v3d::type::geometry::Plane & plane, const boost::shared_ptr<Polygon> & p1, const boost::shared_ptr<Polygon> & p2);

 private:
    std::vector<Vertex> vertices_;
    // one grid covers the whole polygon, so dice() returns true once and false afterwards
    bool diced_ = false;
};

/**
 * Clip a polygon against each of a frustum's planes in turn, keeping what is inside all six.
 * The polygon is rewritten in place.
 **/
void clip(Polygon & poly, const v3d::type::geometry::Frustum & frustum);

};  // namespace v3d::moya
