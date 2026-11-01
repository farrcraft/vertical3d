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

namespace v3d::render::offline::rib {
class ParameterList;
};  // namespace v3d::render::offline::rib

namespace v3d::moya::state {
class RenderContext;
};  // namespace v3d::moya::state

namespace v3d::moya {

class Polygon : public reyes::Primitive {
 public:
    Polygon();
    virtual ~Polygon();

    /**
     * One polygon from the corners that indices names in the position array, with whatever
     * colour, shading normal and texture coordinates the scene gave each corner. An index past
     * the position array is skipped. reyes::Hider::add() gives a vertex left without a
     * colour the current colour, and a vertex left without a normal the plane the polygon lies
     * in.
     **/
    static boost::shared_ptr<Polygon> build(const std::vector<glm::vec3> & points,
        const std::vector<glm::vec3> & colors, const std::vector<glm::vec3> & normals,
        const std::vector<float> & st, const std::vector<unsigned int> & indices);
    /**
     * The polygon RiPolygon describes: the first vertices corners of "P", with "Cs", "N" and
     * "st" read as build() reads them. Both the C interface and the RIB path build a polygon
     * through this.
     *
     * @return null when fewer than three corners have a position
     **/
    static boost::shared_ptr<Polygon> from(const v3d::render::offline::rib::ParameterList & parameters,
        unsigned int vertices);

    /**
     * The polygon as a fan of quadrilaterals about its first vertex, with a triangle last when
     * the count is odd. RI requires a polygon to be convex, so each piece is too. A placed
     * polygon's pieces are placed as it was, with its colour, normal, shading and motion.
     **/
    std::vector<boost::shared_ptr<Polygon>> quads() const;

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
    void split(reyes::Hider & hider, state::RenderContext & rc) override;
    virtual bool dice(boost::shared_ptr<reyes::MicroPolygonGrid> & grid, state::RenderContext & rc);
    /**
     * The same, into a grid of `across` micropolygons a side rather than the context's grid
     * size. The vertices have to be in eye space already, as addPolygon() leaves them.
     **/
    bool dice(boost::shared_ptr<reyes::MicroPolygonGrid> & grid, unsigned int across);

 protected:
    void split(const v3d::type::geometry::Plane & plane, const boost::shared_ptr<Polygon> & p1, const boost::shared_ptr<Polygon> & p2);

 private:
    /**
     * The least length of the cross product of two edges from a polygon's first vertex that gives
     * the polygon a normal.
     **/
    static constexpr float SMALLEST_SPANNED_AREA = 1.0e-8F;

    /**
     * Where an edge whose ends classify on opposite sides of a plane meets it. The two tests round
     * differently, so intersectEdge can find no meeting for an edge that ends a hair past the
     * plane. The edge then meets the plane at the end nearer to it.
     **/
    static glm::vec3 meeting(const v3d::type::geometry::Plane & plane, const glm::vec3 & start, const glm::vec3 & end);

    /**
     * A piece is worth handing back only if it bounds something and is strictly smaller
     * than what it came from on some axis. A split that does not shrink its input would be
     * measured as undiceable again and split again, without end.
     **/
    static bool progress(const boost::shared_ptr<Polygon> & piece, const v3d::type::geometry::AABBox & parent);

    /**
     * A corner of a piece: the vertex it came from, with the position, colour, shading normal and
     * texture coordinates that vertex has. A value the vertex was not given stays unset, so the
     * piece fills it from the primitive's state as its parent was filled.
     **/
    static Vertex carried(const Vertex & from);

    /**
     * The shading normal part of the way from one normal to another, as a unit vector. Two equal
     * normals give that normal back unchanged. Opposite normals meeting half way give zero, which
     * names no direction, so false is returned and the crossing is left without a normal of its
     * own.
     **/
    static bool between(const glm::vec3 & from, const glm::vec3 & to, float along, glm::vec3* normal);

    /**
     * Where an edge meets the plane. Colour, shading normal and texture coordinates are taken as
     * far along the edge as the point is, each only when both ends of the edge have one. The
     * normal is renormalised.
     **/
    static Vertex crossing(const Vertex & start, const Vertex & end, const glm::vec3 & hit);

    /**
     * An edge that crosses the plane. hit is the vertex both halves share, so it goes into each
     * of them. The side start is on decides which half keeps start and which keeps end.
     **/
    static void addCrossingEdge(const Vertex& start, const Vertex& end, const glm::vec3& hit, int side,
        bool first, bool last, const boost::shared_ptr<Polygon>& p1, const boost::shared_ptr<Polygon>& p2);

    /**
     * An edge wholly on one side of the plane, so both its vertices go to the same half.
     *
     * Only the first edge contributes its start and only a non-final edge contributes its end:
     * every other vertex is the end of the edge before it.
     **/
    static void addWholeEdge(const Vertex& start, const Vertex& end, int side, bool first, bool last,
        const boost::shared_ptr<Polygon>& p1, const boost::shared_ptr<Polygon>& p2);

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
