/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "ReyesPrimitive.h"
#include "Vertex.h"
// #include "RenderContext.h"

#include <boost/shared_ptr.hpp>

namespace v3d::moya {
class Plane;
class RenderContext;

class Polygon : public ReyesPrimitive {
 public:
    Polygon();
    virtual ~Polygon();

    /**
     **/
    void addVertex(Vertex vert);

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
     * The plane the polygon lies in, from its first three non collinear vertices, wound
     * the way the vertices are.
     *
     * One value for the whole polygon - SL's Ng. A polygon whose vertices are collinear,
     * or which has fewer than three of them, has no plane and answers zero.
     **/
    glm::vec3 geometricNormal(void) const;

    // reyes methods
    // virtual bool diceable(void) const;
    virtual v3d::type::AABBox bound(void) const;
    virtual void split(RenderContext & rc);
    virtual bool dice(boost::shared_ptr<MicroPolygonGrid> & grid, RenderContext & rc);

 protected:
    void split(const Plane & plane, const boost::shared_ptr<Polygon> & p1, const boost::shared_ptr<Polygon> & p2);

 private:
    std::vector<Vertex> vertices_;
    // one grid covers the whole polygon, so dice() answers true once and false
    // afterwards - the caller loops until it answers false
    bool diced_ = false;
};

};  // namespace v3d::moya
