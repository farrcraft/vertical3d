/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Hider.h"

#include <api/render/offline/Camera.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/MovingTransform.h>
#include <api/render/offline/sl/Types.h>
#include <api/type/geometry/AABBox.h>
#include <api/type/geometry/Frustum.h>
#include <moya/libmoya/Curve.h>
#include <moya/libmoya/Planes.h>
#include <moya/libmoya/Polygon.h>
#include <moya/libmoya/RenderContext.h>
#include <moya/libmoya/Vertex.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Buckets.h"
#include "Primitive.h"
#include "Samples.h"

namespace v3d::moya::reyes {

namespace {

/** An axis aligned box, by its least and greatest corners. **/
struct Bound {
    glm::vec3 min{0.0F};
    glm::vec3 max{0.0F};
};

/**
 * One of the eight corners of a box. Bit 0 of the index picks x, bit 1 picks y and bit 2
 * picks z, each from max when set and from min when clear.
 **/
glm::vec3 boxCorner(const glm::vec3 & min, const glm::vec3 & max, int index) {
    return glm::vec3((index & 1) != 0 ? max.x : min.x,
                     (index & 2) != 0 ? max.y : min.y,
                     (index & 4) != 0 ? max.z : min.z);
}

/**
 * The axis aligned bound of a box carried through an affine matrix.
 *
 * All eight corners go through the matrix. Under a rotation any corner can hold the extreme
 * on an axis, so the two diagonal corners alone can leave part of the box outside.
 **/
Bound transformBound(const glm::mat4x4 & transform, const Bound & object) {
    Bound bound{.min = glm::vec3(std::numeric_limits<float>::max()), .max = glm::vec3(std::numeric_limits<float>::lowest())};
    for (int k = 0; k < 8; k++) {
        const glm::vec3 corner(transform * glm::vec4(boxCorner(object.min, object.max, k), 1.0F));
        bound.min = glm::min(bound.min, corner);
        bound.max = glm::max(bound.max, corner);
    }
    return bound;
}

/**
 * Grow an eye space bound to cover where a moving primitive is at both ends of the shutter,
 * from its object space bound. A still primitive's bound is left as it is.
 **/
void sweep(const v3d::render::offline::MovingTransform & motion, const Bound & object, Bound* swept) {
    if (!motion.moving()) {
        return;
    }
    for (const glm::mat4x4* end : { &motion.open(), &motion.close() }) {
        const Bound at = transformBound(*end, object);
        swept->min = glm::min(swept->min, at.min);
        swept->max = glm::max(swept->max, at.max);
    }
}

/**
 * Give every vertex that brought no colour or normal of its own the primitive's, and every
 * vertex the primitive's plane as its geometric normal.
 *
 * A vertex with no "Cs" takes the primitive's colour, which the surface shader then reads as
 * Cs. The normals go the same way: Ng is the primitive's plane on every vertex, and a vertex
 * that brought no varying "N" shades with it, so a polygon that gives no normals is shaded
 * faceted.
 **/
void fillVertices(Polygon* poly) {
    for (unsigned int i = 0; i < poly->vertexCount(); i++) {
        if (!(*poly)[i].hasColor()) {
            (*poly)[i].color(poly->color());
        }
        (*poly)[i].geometricNormal(poly->normal());
        if (!(*poly)[i].hasNormal()) {
            (*poly)[i].normal(poly->normal());
        }
    }
}

};  // namespace

bool Hider::traces() const {
    return false;
}

void Hider::begin(RenderContext* context) {
    buckets_ = boost::make_shared<Buckets>(Buckets::Sizes{.bucket = glm::uvec2(context->bucketWidth(), context->bucketHeight()),
        .image = glm::uvec2(context->imageWidth(), context->imageHeight())});
}

void Hider::add(const boost::shared_ptr<Polygon> & poly, RenderContext* context) {
    // a hider chosen after the world began has not seen it begin
    if (!buckets_) {
        begin(context);
    }
    // dicing interpolates four corners, so a polygon with more is broken into pieces of four
    // corners or fewer. Each piece is placed as its parent was, and is measured in turn
    if (poly->vertexCount() > 4) {
        for (const boost::shared_ptr<Polygon> & piece : poly->quads()) {
            add(piece, context);
        }
        return;
    }
    fillVertices(poly.get());
    v3d::type::geometry::AABBox bound;
    if (!measure(*poly, poly->bound(), context, &bound)) {
        return;
    }
    const glm::mat4x4 toEye = poly->placement();

    // dicing is done in eye space, so a polygon that will be diced has its vertices moved
    // there now
    if (poly->diceable()) {
        // a normal transforms by the inverse transpose of the matrix that moves the points.
        // The two agree under a rotation and a uniform scale. Under a scale on one axis the
        // point matrix would tilt a normal off its surface
        const glm::mat3x3 toEyeNormal = glm::transpose(glm::inverse(glm::mat3x3(toEye)));
        for (unsigned int i = 0; i < poly->vertexCount(); i++) {
            Vertex pv = poly->vertex(i);
            pv.point(glm::vec3(toEye * glm::vec4(pv.point(), 1.0F)));
            pv.normal(glm::normalize(toEyeNormal * pv.normal()));
            pv.geometricNormal(glm::normalize(toEyeNormal * pv.geometricNormal()));
            (*poly)[i] = pv;
        }
    }

    // the polygon goes in the bucket that holds the upper left corner of its raster bound
    buckets_->add(poly, bound);
}

void Hider::add(const boost::shared_ptr<Curve> & curve, RenderContext* context) {
    if (!buckets_) {
        begin(context);
    }
    // a split shortens a segment and never narrows it, so the segment is measured for dicing
    // by its centre line, and its width only grows the bound it is culled and bucketed by
    v3d::type::geometry::AABBox bound;
    const float reach = 0.5F * std::max(curve->widths().x, curve->widths().y) * widthScale(curve->placement());
    if (!measure(*curve, curve->hull(), context, &bound, reach)) {
        return;
    }
    // dicing is done in eye space, so a segment that will be diced is moved there now
    if (curve->diceable()) {
        curve->toEye();
    }
    buckets_->add(curve, bound);
}

bool Hider::measure(Primitive & primitive, const v3d::type::geometry::AABBox & objectBound, RenderContext* context,
    v3d::type::geometry::AABBox* raster, float reach) {
    const v3d::render::offline::Camera & camera = context->camera();
    v3d::type::geometry::AABBox bound = objectBound;

    glm::vec3 bound_max = bound.max();
    glm::vec3 bound_min = bound.min();

    /*
        The bound is in object space. The placement takes it to eye space, where every
        later calculation on the primitive happens. The placement is the camera's view times
        the object to world transformation. It applies as it stands, with no transpose or
        inverse.
    */
    const glm::mat4x4 toEye = primitive.placement();
    const Bound object{.min = bound_min, .max = bound_max};
    const Bound eye = transformBound(toEye, object);
    bound_min = eye.min;
    bound_max = eye.max;

    // a moving primitive is culled by where it is at both ends of the shutter, as well as by
    // where it is stored. The size test below reads where it is stored, because a split
    // shrinks a primitive and never the distance it travels
    Bound swept = eye;
    sweep(primitive.motion(), object, &swept);

    // a displacement shader may move the surface as far as its bound, so the bound grows by it
    // before the primitive is culled and bucketed. A displacement that goes past the bound is
    // clipped. With no bound the primitive does not grow. The size test reads the surface
    // before it is grown, because the grid it is diced into covers the surface and not the
    // bound. A bound wider than a grid would otherwise be split without end
    const float grown = primitive.shading().displacementBound + reach;
    const glm::vec3 surface_min = bound_min;
    const glm::vec3 surface_max = bound_max;
    bound_min -= glm::vec3(grown);
    bound_max += glm::vec3(grown);
    swept.min -= glm::vec3(grown);
    swept.max += glm::vec3(grown);
    const glm::vec3 & swept_min = swept.min;
    const glm::vec3 & swept_max = swept.max;

    // hither-yon cull
    if ((swept_max[2] > camera.yon && swept_min[2] > camera.yon)  // the bound is wholly beyond the far plane
        || (swept_max[2] < camera.hither && swept_min[2] < camera.hither)) {  // the bound is wholly nearer than the near plane
        return false;
    }

    primitive.diceable(true);

    // eye plane test. A bound that spans the near plane passes the cull above. If it also
    // reaches behind the eye plane, z = 0, it cannot be projected, and is marked undiceable
    // so that it is split
    if (bound_max[2] > camera.hither && bound_min[2] < camera.hither) {
        if (bound_min[2] < 0.0) {
            primitive.diceable(false);
        }
    }

    // viewing frustum cull. The frustum is built from the "screen" coordinate system, which
    // takes eye space to screen space, so it is tested against the eye space bound swept
    // over the shutter. Only the bound is transformed. The primitive stays in object space
    const v3d::type::geometry::Frustum frustum(camera.screen, v3d::type::geometry::Frustum::Depth::MinusOneToOne);
    v3d::type::geometry::AABBox eyeBound;
    eyeBound.extents(swept_min, swept_max);
    if (frustum.intersect(eyeBound) == v3d::type::geometry::Frustum::OUTSIDE) {
        return false;
    }

    // raster applies after screen, so it is on the left: a matrix applies to what is on its
    // right
    const glm::mat4x4 screen = camera.raster;
    // all eight corners of the eye space bound are projected, because under a perspective
    // projection the raster extremes need not lie at two diagonal corners. A corner at or
    // behind the eye has no raster position. It is left out, and the primitive is split. The
    // test on w is the one project() makes before it divides
    glm::vec3 raster_min(std::numeric_limits<float>::max());
    glm::vec3 raster_max(std::numeric_limits<float>::lowest());
    glm::vec3 measured_min(std::numeric_limits<float>::max());
    glm::vec3 measured_max(std::numeric_limits<float>::lowest());
    bool projected = false;
    for (int k = 0; k < 8; k++) {
        const glm::vec3 corner = boxCorner(bound_min, bound_max, k);
        if ((screen * glm::vec4(corner, 1.0F)).w <= v3d::render::offline::sl::SMALLEST_W) {
            primitive.diceable(false);
            continue;
        }
        const glm::vec3 point = project(screen, corner);
        raster_min = glm::min(raster_min, point);
        raster_max = glm::max(raster_max, point);
        projected = true;
        const glm::vec3 inner = project(screen, boxCorner(surface_min, surface_max, k));
        measured_min = glm::min(measured_min, inner);
        measured_max = glm::max(measured_max, inner);
    }
    if (!projected) {
        // no corner is in front of the eye. The primitive is split, and its pieces are
        // bucketed again, so the first bucket holds it until then
        raster_min = glm::vec3(0.0F);
        raster_max = glm::vec3(0.0F);
    }
    bound_min = raster_min;
    bound_max = raster_max;

    bound.extents(bound_min, bound_max);

    // a primitive already marked for splitting needs no size test
    if (primitive.diceable()) {
        // a raster bound wider or taller than one grid's side is split. A grid has context->gridSize()
        // micropolygons, and RI states the shading rate as the area of one in pixels, so a
        // side is sqrt(context->gridSize() * context->shadingRate()) pixels
        const float size = std::sqrt(static_cast<float>(context->gridSize()) * context->shadingRate());
        const glm::vec3 bound_size = measured_max - measured_min;
        if ((bound_size[0] > size) || (bound_size[1] > size)) {
            primitive.diceable(false);
        }
    }
    *raster = bound;
    return true;
}

std::size_t Hider::bucketed() const {
    return buckets_ ? buckets_->primitiveCount() : 0;
}

void Hider::render(RenderContext* context, v3d::render::offline::FrameBuffer* planes) {
    if (!buckets_) {
        begin(context);
    }
    samples_ = boost::make_shared<Samples>(glm::uvec2(planes->width(), planes->height()), context->sampling());
    buckets_->render(*this, *context);
    samples_->resolve(planes, Planes::RED, Planes::COVERAGE, Planes::DEPTH);
}

Samples & Hider::samples() {
    return *samples_;
}

const boost::shared_ptr<Buckets> & Hider::buckets() const {
    return buckets_;
}

};  // namespace v3d::moya::reyes
