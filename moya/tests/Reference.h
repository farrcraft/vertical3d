/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/Compare.h>
#include <api/image/Factory.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/Sampling.h>
#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Reader.h>
#include <moya/libmoya/Planes.h>
#include <moya/libmoya/Polygon.h>
#include <moya/libmoya/RIBHandler.h>
#include <moya/libmoya/RenderContext.h>
#include <moya/libmoya/Vertex.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/operations.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace v3d::moya::reference {

/*
 * The scenes the reference cases render, the pictures they compare against, and the helpers
 * that read a render back. Each scene is built twice, once through the render context and once
 * from a RIB file, and both renders are compared with the same committed picture.
 */

inline constexpr const char* REFERENCE = "data/reference-polygon.png";
inline constexpr const char* RENDERED = "data_out/reference-polygon.png";
inline constexpr const char* RIB_SCENE = "data/reference-polygon.rib";
inline constexpr const char* RIB_RENDERED = "data_out/reference-polygon-rib.png";
inline constexpr const char* SHADED = "data/reference-shaded.png";
inline constexpr const char* SHADED_RENDERED = "data_out/reference-shaded.png";
inline constexpr const char* SHADED_SCENE = "data/reference-shaded.rib";
inline constexpr const char* SHADED_RIB_RENDERED = "data_out/reference-shaded-rib.png";
inline constexpr const char* SAMPLED = "data/reference-sampled.png";
inline constexpr const char* SAMPLED_RENDERED = "data_out/reference-sampled.png";
inline constexpr const char* SAMPLED_SCENE = "data/reference-sampled.rib";
inline constexpr const char* SAMPLED_RIB_RENDERED = "data_out/reference-sampled-rib.png";
inline constexpr const char* SHADOW = "data/reference-shadow.png";
inline constexpr const char* SHADOW_SCENE = "data/reference-shadow.rib";
inline constexpr const char* SHADOW_RIB_RENDERED = "data_out/reference-shadow-rib.png";
inline constexpr const char* FOCUS = "data/reference-focus.png";
inline constexpr const char* FOCUS_RENDERED = "data_out/reference-focus.png";
inline constexpr const char* FOCUS_SCENE = "data/reference-focus.rib";
inline constexpr const char* FOCUS_RIB_RENDERED = "data_out/reference-focus-rib.png";
inline constexpr const char* BUMP = "data/reference-bump.png";
inline constexpr const char* BUMP_RENDERED = "data_out/reference-bump.png";
inline constexpr const char* BUMP_SCENE = "data/reference-bump.rib";
inline constexpr const char* BUMP_RIB_RENDERED = "data_out/reference-bump-rib.png";
inline constexpr const char* AREA = "data/reference-area.png";
inline constexpr const char* AREA_RENDERED = "data_out/reference-area.png";
inline constexpr const char* AREA_SCENE = "data/reference-area.rib";
inline constexpr const char* AREA_RIB_RENDERED = "data_out/reference-area-rib.png";
inline constexpr const char* TRACED_AREA = "data/raytrace-area.png";
inline constexpr const char* TRACED_AREA_RENDERED = "data_out/raytrace-area.png";
inline constexpr const char* TRACED_AREA_SCENE = "data/raytrace-area.rib";
inline constexpr const char* TRACED_AREA_RIB_RENDERED = "data_out/raytrace-area-rib.png";
inline constexpr const char* GLASS = "data/reference-glass.png";
inline constexpr const char* GLASS_RENDERED = "data_out/reference-glass.png";
inline constexpr const char* GLASS_SCENE = "data/reference-glass.rib";
inline constexpr const char* GLASS_RIB_RENDERED = "data_out/reference-glass-rib.png";
inline constexpr const char* DISPLACED = "data/reference-displaced.png";
inline constexpr const char* DISPLACED_RENDERED = "data_out/reference-displaced.png";
inline constexpr const char* DISPLACED_SCENE = "data/reference-displaced.rib";
inline constexpr const char* DISPLACED_RIB_RENDERED = "data_out/reference-displaced-rib.png";
inline constexpr const char* TRACED_DISPLACED = "data/raytrace-displaced.png";
inline constexpr const char* TRACED_DISPLACED_RENDERED = "data_out/raytrace-displaced.png";
inline constexpr const char* TRACED_DISPLACED_SCENE = "data/raytrace-displaced.rib";
inline constexpr const char* TRACED_DISPLACED_RIB_RENDERED = "data_out/raytrace-displaced-rib.png";
inline constexpr const char* TRACED_BUMP = "data/raytrace-bump.png";
inline constexpr const char* TRACED_BUMP_RENDERED = "data_out/raytrace-bump.png";
inline constexpr const char* TRACED_BUMP_SCENE = "data/raytrace-bump.rib";
inline constexpr const char* TRACED_BUMP_RIB_RENDERED = "data_out/raytrace-bump-rib.png";
inline constexpr const char* MOTION = "data/reference-motion.png";
inline constexpr const char* MOTION_RENDERED = "data_out/reference-motion.png";
inline constexpr const char* MOTION_SCENE = "data/reference-motion.rib";
inline constexpr const char* MOTION_RIB_RENDERED = "data_out/reference-motion-rib.png";
inline constexpr const char* TRIANGLE = "data/raytrace-triangle.png";
inline constexpr const char* TRIANGLE_RENDERED = "data_out/raytrace-triangle.png";
inline constexpr const char* TRIANGLE_SCENE = "data/raytrace-triangle.rib";
inline constexpr const char* TRIANGLE_RIB_RENDERED = "data_out/raytrace-triangle-rib.png";
inline constexpr const char* TRACED_SHADED = "data/raytrace-shaded.png";
inline constexpr const char* TRACED_SHADED_RENDERED = "data_out/raytrace-shaded.png";
inline constexpr const char* TRACED_SHADED_SCENE = "data/raytrace-shaded.rib";
inline constexpr const char* TRACED_SHADED_RIB_RENDERED = "data_out/raytrace-shaded-rib.png";
inline constexpr const char* TRACED_SAMPLED = "data/raytrace-sampled.png";
inline constexpr const char* TRACED_SAMPLED_RENDERED = "data_out/raytrace-sampled.png";
inline constexpr const char* TRACED_SAMPLED_SCENE = "data/raytrace-sampled.rib";
inline constexpr const char* TRACED_SAMPLED_RIB_RENDERED = "data_out/raytrace-sampled-rib.png";
inline constexpr const char* TRACED_FOCUS = "data/raytrace-focus.png";
inline constexpr const char* TRACED_FOCUS_RENDERED = "data_out/raytrace-focus.png";
inline constexpr const char* TRACED_FOCUS_SCENE = "data/raytrace-focus.rib";
inline constexpr const char* TRACED_FOCUS_RIB_RENDERED = "data_out/raytrace-focus-rib.png";
inline constexpr const char* TRACED_MOTION = "data/raytrace-motion.png";
inline constexpr const char* TRACED_MOTION_RENDERED = "data_out/raytrace-motion.png";
inline constexpr const char* TRACED_MOTION_SCENE = "data/raytrace-motion.rib";
inline constexpr const char* TRACED_MOTION_RIB_RENDERED = "data_out/raytrace-motion-rib.png";
inline constexpr const char* SPHERES = "data/raytrace-spheres.png";
inline constexpr const char* SPHERES_RENDERED = "data_out/raytrace-spheres.png";
inline constexpr const char* SPHERES_SCENE = "data/raytrace-spheres.rib";
inline constexpr const char* SPHERES_RIB_RENDERED = "data_out/raytrace-spheres-rib.png";
/** The two hiders, for a property both are expected to have. **/
inline constexpr const char* HIDERS[] = { "hidden", "raytrace" };
/**
 * One sample at each pixel centre under a one pixel box, which the film gives back exactly.
 * A reference drawn this way checks the hider and the shading. reference-sampled checks the
 * sampling.
 **/
void pixelCentres(v3d::moya::RenderContext & rc);

v3d::moya::Vertex vertex(float x, float y, float z);

/*
    A quad facing the camera, asymmetric about both axes so that a flipped picture is a
    failing one. The screen window is the default for a 4:3 frame, [-4/3, 4/3] by [-1, 1].
    The raster transform puts its origin at the upper left corner.

    Regenerate this reference when a change to shading or sampling alters the picture on
    purpose. Any other change to the picture fails the test.
*/
void scene(v3d::moya::RenderContext & rc);

/**
 * A vertex with a shading normal of its own, so that a quad's cosine falloff is a gradient
 * rather than one value.
 **/
v3d::moya::Vertex shaded(float x, float y, float z, const glm::vec3 & normal);

/**
 * A quad wound clockwise as the camera sees it, so its geometric normal faces the camera as its
 * shading normals do. Two-argument faceforward turns a normal by the geometric one.
 **/
void quad(v3d::moya::RenderContext & rc, float left, float right,
    const glm::vec3 & outer, const glm::vec3 & inner);

/**
 * One parameter as a scene wrote it, typed the way the reader would have typed it.
 *
 * The list is an out parameter, as in rib::arguments. A ParameterList is a map, and returning
 * one by value copies every parameter in it.
 **/
void put(v3d::render::offline::rib::ParameterList* list, const std::string & name,
    v3d::render::offline::rib::Declaration::Type type, const std::vector<float> & values);

/*
    The same scene as data/reference-shaded.rib, built through the render context instead
    of read from a file. Two surfaces under two lights: a matte one and a plastic one, a
    distant light and a point light close enough for its falloff to show. It is sampled at
    the RI defaults unless the caller says otherwise.
*/
void shadedScene(v3d::moya::RenderContext & rc);

/*
    The same scene as data/reference-focus.rib: a red quad on the plane of focus four units
    out and a blue one ten units out, through a perspective camera. The lens is the file's,
    or a pinhole.
*/
void focusScene(v3d::moya::RenderContext & rc, bool lens);

/** A row, and the columns along it from the first to the last, inclusive. **/
struct Run {
    unsigned int row = 0;
    unsigned int from = 0;
    unsigned int to = 0;
};

/**
 * How many pixels along a row are partly covered. That count is the width of an edge.
 **/
unsigned int partial(const v3d::render::offline::FrameBuffer & planes, unsigned int coverage, const Run & run);

/** The first and last columns of a block, inclusive. **/
struct Columns {
    unsigned int from = 0;
    unsigned int to = 0;
};

/**
 * The largest difference between two frames over every plane of a block of columns.
 **/
float largest(const v3d::render::offline::FrameBuffer & first, const v3d::render::offline::FrameBuffer & second,
    const Columns & columns);

/**
 * The orthographic camera of the other references, with the shutter open from 0 to 1.
 **/
void motionCamera(v3d::moya::RenderContext & rc);

void quadAt(v3d::moya::RenderContext & rc, const glm::vec3 (&corners)[4]);

/*
    The same scene as data/reference-motion.rib: a quad sliding right and a quad turning a
    quarter about its centre, each across the whole shutter.
*/
void motionScene(v3d::moya::RenderContext & rc);

/**
 * The coverage of a quad that slid twelve pixels right while the shutter was open, along the
 * row through its middle. Its left edge leaves pixels 8 to 19 one after another, and its
 * right edge reaches pixels 32 to 43. Under a one pixel box the coverage climbs a twelfth a
 * pixel from 8, falls a twelfth a pixel from 32, and is whole between.
 **/
void checkRamp(const v3d::render::offline::FrameBuffer & planes, unsigned int coverage, unsigned int row);

/**
 * A rendered image against a committed one, writing what it rendered when they differ.
 **/
void check(const boost::shared_ptr<v3d::image::Image> & rendered, const char* reference,
    const char* output);

/** A scene file, and the hider to render it under, or null for the one the file names. **/
struct Scene {
    const char* file = nullptr;
    const char* hider = nullptr;
};

/**
 * A frame read from a file, under the hider the file names unless the caller names one first.
 **/
boost::shared_ptr<v3d::render::offline::FrameBuffer> read(const Scene & scene);

boost::shared_ptr<v3d::image::Image> picture(const v3d::moya::RenderContext & rc);

/**
 * The camera the ray traced references share: orthographic over the default 4:3 screen
 * window, with the world origin one unit in front of the eye. The world is not begun, so a
 * caller can still name an imager.
 **/
void raytraceCamera(v3d::moya::RenderContext & rc);

void background(v3d::moya::RenderContext & rc, const glm::vec3 & colour);

/*
    The same scene as data/raytrace-triangle.rib: a triangle asymmetric about both axes, so a
    flipped picture is a failing one, drawn by the shader that means no shading.
*/
void triangleScene(v3d::moya::RenderContext & rc);

/*
    The same scene as data/raytrace-shaded.rib: a matte floor and a plastic panel, three
    lights of three kinds, and a shadow the panel casts across the floor. It is sampled at the
    RI defaults unless the caller says otherwise.
*/
void raytraceShadedScene(v3d::moya::RenderContext & rc);

/*
    The same scene as data/raytrace-spheres.rib: a metal sphere and a glass one over a checked
    floor, in front of a wall, through a perspective camera at the origin and sampled at the RI
    defaults.
*/
void spheresScene(v3d::moya::RenderContext & rc);

/**
 * A quad sliding half a unit right across the whole shutter, sampled finely under a one pixel
 * box, which is twelve pixels at this frame.
 **/
void slidingQuad(v3d::moya::RenderContext & rc);

};  // namespace v3d::moya::reference
