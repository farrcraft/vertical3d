/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/rib/Bases.h>
#include <api/type/geometry/AABBox.h>
#include <moya/libmoya/reyes/Primitive.h>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace v3d::moya {

class RenderContext;

/**
 * One cubic Bezier segment of a curve, as the reyes hider draws it: a flat ribbon facing the
 * camera, of a width that varies from the segment's start to its end.
 *
 * Its control points start in object space. reyes::Hider::add() measures a segment by its
 * centre line, so a ribbon wider than a grid is diced into wider micropolygons rather than
 * split, and moves a segment it will dice into eye space, scaling the width with it. A segment
 * longer than a grid is split in two at its middle, and each half is measured again. A segment
 * split so often that its halves still do not shrink, as one that crosses the eye plane does
 * not, is dropped.
 *
 * A grid runs along the segment in its first index and across the ribbon in its second. Its s
 * is across the ribbon and its t along the segment, as RI gives a curve's u and v. The normal
 * at each point faces the eye, square to the curve.
 **/
class Curve final : public reyes::Primitive {
 public:
    using Controls = v3d::render::offline::rib::Segment;

    /**
     * @param widths the ribbon's width at the segment's start and at its end, in the space of
     *        the control points
     **/
    Curve(const Controls & controls, const glm::vec2 & widths);

    const Controls & controls() const;
    const glm::vec2 & widths() const;

    /**
     * Move the control points by the placement into eye space, and scale the widths as the
     * placement scales.
     **/
    void toEye();

    /** A box that holds the centre line: its control points' box. **/
    v3d::type::geometry::AABBox hull(void) const;

    // reyes::Primitive
    /** The hull grown by half the widest width, which holds the ribbon. **/
    v3d::type::geometry::AABBox bound(void) const override;
    void split(reyes::Hider & hider, RenderContext & rc) override;
    bool dice(boost::shared_ptr<reyes::MicroPolygonGrid> & grid, RenderContext & rc) override;
    /**
     * The same, into a grid of `across` micropolygons a side rather than the context's grid
     * size. The control points have to be in eye space already.
     **/
    bool dice(boost::shared_ptr<reyes::MicroPolygonGrid> & grid, unsigned int across);

 private:
    Controls controls_;
    glm::vec2 widths_;
    unsigned int splits_ = 0;
    // one grid covers the whole segment, so dice() returns true once and false afterwards
    bool diced_ = false;
};

/**
 * How much a transformation scales a width: the cube root of how much it scales a volume. It
 * is exact for a uniform scale, and the geometric mean of the three scales for any other.
 **/
float widthScale(const glm::mat4 & transform);

};  // namespace v3d::moya
