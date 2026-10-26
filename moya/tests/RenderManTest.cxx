/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/Sampling.h>
#include <moya/libmoya/Planes.h>
#include <moya/libmoya/RIBHandler.h>
#include <moya/libmoya/RenderContext.h>
#include <moya/libmoya/RenderMan.h>
#include <moya/libmoya/Shading.h>

#include <cmath>
#include <string>
#include <type_traits>
#include <utility>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * The context the C entry points write to. RiGetContext is RI's own query for it, and the
 * handle it returns is the handler that holds the context.
 **/
v3d::moya::RenderContext & context() {
    return static_cast<v3d::moya::RIBHandler*>(RiGetContext())->context();
}

/**
 * Each C entry point passes its request to the handler the RIB reader calls, so the two reach
 * the same graphics state by the same rules.
 *
 * A parameter a C caller passes is typed by what RiDeclare said, the same table a file's
 * Declare fills.
 **/
BOOST_AUTO_TEST_CASE(renderman_surface_and_lights_test) {
    RiBegin(RI_NULL);
    RiFormat(64, 48, 1.0F);
    RiClipping(1.0F, 100.0F);
    RiWorldBegin();

    RtFloat intensity = 0.5F;
    RtLightHandle key = RiLightSource(const_cast<char*>("distantlight"),
        const_cast<char*>("intensity"), &intensity, RI_NULL);
    BOOST_REQUIRE(key != nullptr);

    RtFloat roughness = 0.2F;
    RiSurface(const_cast<char*>("plastic"),
        const_cast<char*>("roughness"), &roughness, RI_NULL);

    v3d::moya::Shading shading = context().shading();
    BOOST_REQUIRE(shading.surface);
    BOOST_CHECK_EQUAL(shading.surface->name(), "plastic");
    BOOST_REQUIRE_EQUAL(shading.lights.size(), 1U);
    BOOST_CHECK_EQUAL(shading.lights[0].shader->name(), "distantlight");

    // the handle the interface gave back is the one that turns the light off again
    RiIlluminate(key, RI_FALSE);
    BOOST_CHECK_EQUAL(context().shading().lights.size(), 0U);

    RiWorldEnd();
    RiEnd();
}

/**
 * A parameter the scene declared itself is typed by that declaration, so a shader parameter
 * the standard does not define can still be bound.
 **/
BOOST_AUTO_TEST_CASE(renderman_declare_test) {
    RiBegin(RI_NULL);
    RiDeclare(const_cast<char*>("squish"), const_cast<char*>("uniform float"));

    RtFloat squish = 5.0F;
    RiSurface(const_cast<char*>("matte"), const_cast<char*>("squish"), &squish, RI_NULL);

    // matte declares no "squish", so the parameter is dropped and the surface is still matte
    const v3d::moya::Shading shading = context().shading();
    BOOST_REQUIRE(shading.surface);
    BOOST_CHECK_EQUAL(shading.surface->name(), "matte");
    RiEnd();
}

/**
 * The sampling requests reach the same context from C. A filter is named by its function
 * there, and one the interface does not declare leaves the filter the context had.
 **/
BOOST_AUTO_TEST_CASE(renderman_sampling_test) {
    RiBegin(RI_NULL);
    RiPixelSamples(4.0F, 4.0F);
    RiPixelFilter(RiCatmullRomFilter, 3.0F, 3.0F);
    RiDepthOfField(8.0F, 0.1F, 3.0F);
    RiShutter(0.0F, 0.5F);

    const v3d::render::offline::Sampling & sampling = context().sampling();
    BOOST_CHECK_EQUAL(sampling.samples.x, 4U);
    BOOST_CHECK_EQUAL(sampling.samples.y, 4U);
    BOOST_CHECK(sampling.filter == v3d::render::offline::Filter::CatmullRom);
    BOOST_CHECK_EQUAL(sampling.width.x, 3.0F);
    BOOST_CHECK_EQUAL(sampling.fstop, 8.0F);
    BOOST_CHECK_EQUAL(sampling.focalLength, 0.1F);
    BOOST_CHECK_EQUAL(sampling.focalDistance, 3.0F);
    BOOST_CHECK_EQUAL(sampling.shutter.x, 0.0F);
    BOOST_CHECK_EQUAL(sampling.shutter.y, 0.5F);

    RiPixelFilter(nullptr, 1.0F, 1.0F);
    BOOST_CHECK(context().sampling().filter == v3d::render::offline::Filter::CatmullRom);
    BOOST_CHECK_EQUAL(context().sampling().width.x, 3.0F);
    RiEnd();
}

/**
 * A width or a resolution the RIB reader refuses is refused through the C interface too, and
 * the context keeps what it had. A width that is not a number would otherwise reach the film
 * and place a sample at an undefined pixel index. A side above largestResolution is refused,
 * and an aspect that is not a number is square pixels.
 **/
BOOST_AUTO_TEST_CASE(renderman_refuses_what_the_reader_refuses_test) {
    RiBegin(RI_NULL);
    RiPixelFilter(RiBoxFilter, 2.0F, 2.0F);
    RiPixelFilter(RiGaussianFilter, std::nanf(""), 1.0F);
    RiPixelFilter(RiGaussianFilter, 0.0F, 1.0F);
    RiPixelFilter(RiGaussianFilter, 1.0F, -1.0F);
    BOOST_CHECK(context().sampling().filter == v3d::render::offline::Filter::Box);
    BOOST_CHECK_EQUAL(context().sampling().width.x, 2.0F);
    BOOST_CHECK_EQUAL(context().sampling().width.y, 2.0F);

    RiFormat(32, 16, 2.0F);
    RiFormat(70000, 16, 1.0F);
    RiFormat(32, 65537, 1.0F);
    BOOST_CHECK_EQUAL(context().imageWidth(), 32U);
    BOOST_CHECK_EQUAL(context().imageHeight(), 16U);
    BOOST_CHECK_EQUAL(context().pixelAspect(), 2.0F);
    RiFormat(32, 16, std::nanf(""));
    BOOST_CHECK_EQUAL(context().pixelAspect(), 1.0F);
    RiWorldBegin();
    RiWorldEnd();
    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = context().picture();
    BOOST_REQUIRE(planes);
    BOOST_CHECK_EQUAL(planes->width(), 32U);
    BOOST_CHECK_EQUAL(planes->height(), 16U);
    RiEnd();
}

/**
 * RI reads a side of zero or less as the device's default for that side, which for moya is
 * 320 by 240. The other side keeps the size the call named.
 **/
BOOST_AUTO_TEST_CASE(renderman_format_nonpositive_side_is_the_default_test) {
    RiBegin(RI_NULL);
    RiFormat(-1, 16, 1.0F);
    BOOST_CHECK_EQUAL(context().imageWidth(), 320U);
    BOOST_CHECK_EQUAL(context().imageHeight(), 16U);

    RiFormat(32, 0, 1.0F);
    BOOST_CHECK_EQUAL(context().imageWidth(), 32U);
    BOOST_CHECK_EQUAL(context().imageHeight(), 240U);
    RiEnd();
}

/**
 * RiAreaLightSource keeps the geometry declared after it in the same attribute block. A unit
 * quad inside the block joins the light, and one after the block does not.
 **/
BOOST_AUTO_TEST_CASE(renderman_an_area_light_keeps_its_geometry_test) {
    RiBegin(RI_NULL);
    RiFormat(64, 48, 1.0F);
    RiClipping(1.0F, 100.0F);
    RiWorldBegin();
    RiAttributeBegin();
    RtLightHandle area = RiAreaLightSource(const_cast<char*>("pointlight"), RI_NULL);
    BOOST_REQUIRE(area != nullptr);
    RtPoint quad[4] = { { 0.0F, 0.0F, 5.0F }, { 1.0F, 0.0F, 5.0F }, { 1.0F, 1.0F, 5.0F }, { 0.0F, 1.0F, 5.0F } };
    RiPolygon(4, const_cast<char*>("P"), static_cast<RtPointer>(quad), RI_NULL);
    BOOST_REQUIRE(context().areaLight());
    RiAttributeEnd();
    BOOST_CHECK(!context().areaLight());
    // the block's end switched the light off, as it does any light made inside one
    RiIlluminate(area, RI_TRUE);
    RiPolygon(4, const_cast<char*>("P"), static_cast<RtPointer>(quad), RI_NULL);

    const v3d::moya::Shading shading = context().shading();
    BOOST_REQUIRE_EQUAL(shading.lights.size(), 1U);
    BOOST_REQUIRE(shading.lights[0].area);
    BOOST_CHECK_CLOSE(shading.lights[0].area->area(), 1.0F, 0.01F);
    RiWorldEnd();
    RiEnd();
}

/**
 * RiDisplacement names the displacement shader for what follows, and RiAttribute's
 * "displacementbound" bounds how far it moves a point. Both reach the shading state the
 * context gives a primitive. The bound is stated in object space, which a scale of two makes
 * twice as large in camera space.
 **/
BOOST_AUTO_TEST_CASE(renderman_a_displacement_reaches_the_context_test) {
    RiBegin(RI_NULL);
    RiFormat(64, 48, 1.0F);
    RiClipping(1.0F, 100.0F);
    RiWorldBegin();
    RiDeclare(const_cast<char*>("sphere"), const_cast<char*>("float"));
    RiScale(2.0F, 2.0F, 2.0F);
    RtFloat sphere = 0.3F;
    RiAttribute(const_cast<char*>("displacementbound"), const_cast<char*>("sphere"), &sphere, RI_NULL);
    RtFloat amount = 0.1F;
    RiDeclare(const_cast<char*>("amount"), const_cast<char*>("float"));
    // lift is a fixture beside the suite's scenes
    RtString path = const_cast<char*>("data:&");
    RiOption(const_cast<char*>("searchpath"), const_cast<char*>("shader"), &path, RI_NULL);
    RiDisplacement(const_cast<char*>("lift"), const_cast<char*>("amount"), &amount, RI_NULL);
    const v3d::moya::Shading shading = context().shading();
    BOOST_REQUIRE(shading.displacement.shader);
    BOOST_CHECK_EQUAL(shading.displacement.shader->name(), "lift");
    BOOST_CHECK_CLOSE(shading.displacementBound, 0.6F, 0.01F);
    RiWorldEnd();
    RiEnd();
}

/**
 * A quad that covers an 8 by 6 picture, coloured red at every corner through "Cs", at one
 * sample a pixel. Only the colour a polygon's own parameter gives can make the picture red,
 * because the colour in force is white.
 **/
const RtPoint COVER[4] = { { -5.0F, -5.0F, 5.0F }, { 5.0F, -5.0F, 5.0F }, { 5.0F, 5.0F, 5.0F }, { -5.0F, 5.0F, 5.0F } };
const RtPoint RED[4] = { { 1.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F } };

void beginCovered() {
    RiBegin(RI_NULL);
    RiFormat(8, 6, 1.0F);
    RiPixelSamples(1.0F, 1.0F);
    RiPixelFilter(RiBoxFilter, 1.0F, 1.0F);
    RiClipping(1.0F, 100.0F);
    RiWorldBegin();
}

void checkRed() {
    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = context().picture();
    BOOST_REQUIRE(planes);
    BOOST_CHECK_CLOSE(planes->value(v3d::moya::Planes::RED, 4, 3), 1.0F, 0.01F);
    BOOST_CHECK_SMALL(planes->value(v3d::moya::Planes::GREEN, 4, 3), 0.001F);
}

/**
 * RiPolygon reads every parameter the RIB path reads, so a colour given per vertex reaches the
 * picture.
 **/
BOOST_AUTO_TEST_CASE(renderman_polygon_reads_its_colours_test) {
    beginCovered();
    RiPolygon(4, RI_P, const_cast<RtPoint*>(COVER), RI_CS, const_cast<RtPoint*>(RED), RI_NULL);
    RiWorldEnd();
    checkRed();
    RiEnd();
}

/**
 * RiPolygonV is the form RiPolygon forwards to, and draws the same polygon.
 **/
BOOST_AUTO_TEST_CASE(renderman_polygon_vector_form_draws_test) {
    beginCovered();
    RtToken tokens[2] = { RI_P, RI_CS };
    RtPointer parms[2] = { const_cast<RtPoint*>(COVER), const_cast<RtPoint*>(RED) };
    RiPolygonV(4, 2, tokens, parms);
    RiWorldEnd();
    checkRed();
    RiEnd();
}

/**
 * A polygon after the first in a motion block is the first one deforming, which moya does not
 * draw, as it does not from a file. The later polygon is green and nearer the camera, so it
 * would cover the red one if it were drawn.
 **/
BOOST_AUTO_TEST_CASE(renderman_a_deforming_polygon_is_not_drawn_test) {
    const RtPoint nearer[4] = { { -5.0F, -5.0F, 2.0F }, { 5.0F, -5.0F, 2.0F }, { 5.0F, 5.0F, 2.0F }, { -5.0F, 5.0F, 2.0F } };
    const RtPoint green[4] = { { 0.0F, 1.0F, 0.0F }, { 0.0F, 1.0F, 0.0F }, { 0.0F, 1.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } };
    beginCovered();
    RiMotionBegin(2, 0.0, 1.0);
    RiPolygon(4, RI_P, const_cast<RtPoint*>(COVER), RI_CS, const_cast<RtPoint*>(RED), RI_NULL);
    RiPolygon(4, RI_P, const_cast<RtPoint*>(nearer), RI_CS, const_cast<RtPoint*>(green), RI_NULL);
    RiMotionEnd();
    RiWorldEnd();
    checkRed();
    RiEnd();
}

/**
 * RiPointsPolygons draws its faces, and reads a vertex value at each point the faces index.
 * Two triangles that share a diagonal make the covering quad.
 **/
BOOST_AUTO_TEST_CASE(renderman_points_polygons_draws_test) {
    beginCovered();
    RtInt nverts[2] = { 3, 3 };
    RtInt verts[6] = { 0, 1, 2, 0, 2, 3 };
    RiPointsPolygons(2, nverts, verts, RI_P, const_cast<RtPoint*>(COVER), RI_CS, const_cast<RtPoint*>(RED), RI_NULL);
    RiWorldEnd();
    checkRed();
    RiEnd();
}

/**
 * RiSphere draws under the ray hider, as a file's Sphere does. A large sphere in front of the
 * camera covers the picture in the colour in force.
 **/
BOOST_AUTO_TEST_CASE(renderman_sphere_draws_under_the_ray_hider_test) {
    RiBegin(RI_NULL);
    RiHider(const_cast<char*>("raytrace"), RI_NULL);
    RiFormat(8, 6, 1.0F);
    RiPixelSamples(1.0F, 1.0F);
    RiPixelFilter(RiBoxFilter, 1.0F, 1.0F);
    RiClipping(1.0F, 100.0F);
    RiWorldBegin();
    RtColor red = { 1.0F, 0.0F, 0.0F };
    RiColor(red);
    RiTranslate(0.0F, 0.0F, 20.0F);
    RiSphere(10.0F, -10.0F, 10.0F, 360.0F, RI_NULL);
    RiWorldEnd();
    checkRed();
    RiEnd();
}

/**
 * RiProjection reads "fov" as a pointer to a float, as RI passes every parameter value.
 **/
BOOST_AUTO_TEST_CASE(renderman_projection_reads_its_fov_test) {
    v3d::moya::RenderContext expected("");
    expected.projection("perspective", 45.0F);

    RiBegin(RI_NULL);
    RtFloat fov = 45.0F;
    RiProjection(RI_PERSPECTIVE, RI_FOV, &fov, RI_NULL);
    BOOST_TEST((context().coordinateSystem("screen") == expected.coordinateSystem("screen")));
    RiEnd();
}

/**
 * RiProjection(RI_NULL) applies no projection, so camera space is screen space.
 * "orthographic" is a projection of its own.
 **/
BOOST_AUTO_TEST_CASE(renderman_null_projection_is_the_identity_test) {
    RiBegin(RI_NULL);
    RiProjection(RI_NULL, RI_NULL);
    BOOST_TEST((context().coordinateSystem("screen") == glm::mat4x4(1.0F)));
    RiProjection(RI_ORTHOGRAPHIC, RI_NULL);
    BOOST_TEST((context().coordinateSystem("screen") != glm::mat4x4(1.0F)));
    RiEnd();
}

/**
 * RiDisplayV is the form RiDisplay forwards to, and names the display.
 **/
BOOST_AUTO_TEST_CASE(renderman_display_vector_form_names_the_display_test) {
    RiBegin(RI_NULL);
    RiDisplayV(const_cast<char*>("picture.tif"), RI_FILE, RI_RGB, 0, nullptr, nullptr);
    BOOST_CHECK_EQUAL(context().displayName(), "picture.tif");
    RiEnd();
}

/**
 * RiContext makes the context a handle names active again. The handle stays good while other
 * contexts begin after it, and RiEnd then ends the context it made active.
 **/
BOOST_AUTO_TEST_CASE(renderman_context_switches_to_its_handle_test) {
    RiBegin(RI_NULL);
    RiFormat(32, 16, 1.0F);
    RtContextHandle outer = RiGetContext();
    RiBegin(RI_NULL);
    RiFormat(64, 48, 1.0F);

    RiContext(outer);
    BOOST_CHECK_EQUAL(context().imageWidth(), 32U);
    RiEnd();
    BOOST_CHECK_EQUAL(context().imageWidth(), 64U);
    RiEnd();
}

/**
 * Every token RI declares holds RI's string for it, so a caller can pass any of them.
 **/
BOOST_AUTO_TEST_CASE(renderman_every_token_holds_its_name_test) {
    const std::pair<RtToken, const char*> tokens[] = {
        { RI_FRAMEBUFFER, "framebuffer" }, { RI_FILE, "file" },
        { RI_RGB, "rgb" }, { RI_RGBA, "rgba" }, { RI_RGBZ, "rgbz" }, { RI_RGBAZ, "rgbaz" },
        { RI_A, "a" }, { RI_Z, "z" }, { RI_AZ, "az" },
        { RI_PERSPECTIVE, "perspective" }, { RI_ORTHOGRAPHIC, "orthographic" },
        { RI_HIDDEN, "hidden" }, { RI_PAINT, "paint" },
        { RI_CONSTANT, "constant" }, { RI_SMOOTH, "smooth" },
        { RI_FLATNESS, "flatness" }, { RI_FOV, "fov" },
        { RI_AMBIENTLIGHT, "ambientlight" }, { RI_POINTLIGHT, "pointlight" },
        { RI_DISTANTLIGHT, "distantlight" }, { RI_SPOTLIGHT, "spotlight" },
        { RI_INTENSITY, "intensity" }, { RI_LIGHTCOLOR, "lightcolor" }, { RI_FROM, "from" },
        { RI_TO, "to" }, { RI_CONEANGLE, "coneangle" }, { RI_CONEDELTAANGLE, "conedeltaangle" },
        { RI_BEAMDISTRIBUTION, "beamdistribution" },
        { RI_MATTE, "matte" }, { RI_METAL, "metal" }, { RI_SHINYMETAL, "shinymetal" },
        { RI_PLASTIC, "plastic" }, { RI_PAINTEDPLASTIC, "paintedplastic" },
        { RI_KA, "Ka" }, { RI_KD, "Kd" }, { RI_KS, "Ks" }, { RI_ROUGHNESS, "roughness" },
        { RI_KR, "Kr" }, { RI_TEXTURENAME, "texturename" }, { RI_SPECULARCOLOR, "specularcolor" },
        { RI_DEPTHCUE, "depthcue" }, { RI_FOG, "fog" }, { RI_BUMPY, "bumpy" },
        { RI_MINDISTANCE, "mindistance" }, { RI_MAXDISTANCE, "maxdistance" },
        { RI_BACKGROUND, "background" }, { RI_DISTANCE, "distance" }, { RI_AMPLITUDE, "amplitude" },
        { RI_RASTER, "raster" }, { RI_SCREEN, "screen" }, { RI_CAMERA, "camera" },
        { RI_WORLD, "world" }, { RI_OBJECT, "object" },
        { RI_INSIDE, "inside" }, { RI_OUTSIDE, "outside" }, { RI_LH, "lh" }, { RI_RH, "rh" },
        { RI_P, "P" }, { RI_PZ, "Pz" }, { RI_PW, "Pw" }, { RI_N, "N" }, { RI_NP, "Np" },
        { RI_CS, "Cs" }, { RI_OS, "Os" }, { RI_S, "s" }, { RI_T, "t" }, { RI_ST, "st" },
        { RI_BILINEAR, "bilinear" }, { RI_BICUBIC, "bicubic" },
        { RI_LINEAR, "linear" }, { RI_CUBIC, "cubic" },
        { RI_PRIMITIVE, "primitive" }, { RI_INTERSECTION, "intersection" },
        { RI_UNION, "union" }, { RI_DIFFERENCE, "difference" },
        { RI_PERIODIC, "periodic" }, { RI_NONPERIODIC, "nonperiodic" },
        { RI_CLAMP, "clamp" }, { RI_BLACK, "black" },
        { RI_IGNORE, "ignore" }, { RI_PRINT, "print" }, { RI_ABORT, "abort" },
        { RI_HANDLER, "handler" },
        { RI_COMMENT, "comment" }, { RI_STRUCTURE, "structure" }, { RI_VERBATIM, "verbatim" },
        { RI_IDENTIFIER, "identifier" }, { RI_NAME, "name" }, { RI_SHADINGGROUP, "shadinggroup" },
        { RI_WIDTH, "width" }, { RI_CONSTANTWIDTH, "constantwidth" },
    };
    for (const auto& [token, name] : tokens) {
        BOOST_TEST_CONTEXT(name) {
            BOOST_TEST_REQUIRE((token != nullptr));
            BOOST_TEST(std::string(token) == name);
        }
    }
}

/**
 * Each basis matrix holds RI's values for it. Every row of the B-spline basis is divided by
 * six, and every row of the Catmull-Rom basis by two.
 **/
BOOST_AUTO_TEST_CASE(renderman_every_basis_holds_its_matrix_test) {
    const float sixth = 1.0F / 6.0F;
    const RtBasis bezier = { { -1, 3, -3, 1 }, { 3, -6, 3, 0 }, { -3, 3, 0, 0 }, { 1, 0, 0, 0 } };
    const RtBasis bspline = { { -sixth, 3 * sixth, -3 * sixth, sixth }, { 3 * sixth, -6 * sixth, 3 * sixth, 0 },
        { -3 * sixth, 0, 3 * sixth, 0 }, { sixth, 4 * sixth, sixth, 0 } };
    const RtBasis catmullRom = { { -0.5F, 1.5F, -1.5F, 0.5F }, { 1, -2.5F, 2, -0.5F }, { -0.5F, 0, 0.5F, 0 },
        { 0, 1, 0, 0 } };
    const RtBasis hermite = { { 2, 1, -2, 1 }, { -3, -2, 3, -1 }, { 0, 1, 0, 0 }, { 1, 0, 0, 0 } };
    const RtBasis power = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
    const std::pair<const RtFloat*, const RtFloat*> bases[] = {
        { &RiBezierBasis[0][0], &bezier[0][0] }, { &RiBSplineBasis[0][0], &bspline[0][0] },
        { &RiCatmullRomBasis[0][0], &catmullRom[0][0] }, { &RiHermiteBasis[0][0], &hermite[0][0] },
        { &RiPowerBasis[0][0], &power[0][0] },
    };
    for (const auto& [actual, expected] : bases) {
        for (int each = 0; each < 16; each++) {
            BOOST_TEST(actual[each] == expected[each], boost::test_tools::tolerance(0.00001F));
        }
    }
}

/**
 * A null space is ignored by RiCoordinateSystem and RiCoordSysTransform, so the current
 * transformation is unchanged.
 **/
BOOST_AUTO_TEST_CASE(renderman_a_null_coordinate_system_is_ignored_test) {
    RiBegin(RI_NULL);
    RiScale(2.0F, 2.0F, 2.0F);
    RiCoordinateSystem(RI_NULL);
    RiCoordSysTransform(RI_NULL);
    RiCoordinateSystem(const_cast<char*>("kept"));
    BOOST_TEST((context().coordinateSystem("kept") == glm::scale(glm::mat4x4(1.0F), glm::vec3(2.0F))));
    RiEnd();
}

/**
 * RI's own name for a predefined space is refused by RiCoordinateSystem, so the camera a scene
 * set up is the one the hiders read.
 **/
BOOST_AUTO_TEST_CASE(renderman_a_predefined_space_cannot_be_renamed_test) {
    RiBegin(RI_NULL);
    RiFormat(64, 48, 1.0F);
    RiProjection(RI_PERSPECTIVE, RI_NULL);
    RiTranslate(0.0F, 0.0F, 5.0F);
    RiWorldBegin();
    const glm::mat4x4 raster = context().coordinateSystem("raster");
    const glm::mat4x4 camera = context().coordinateSystem("camera");
    RiScale(3.0F, 3.0F, 3.0F);
    RiCoordinateSystem(RI_RASTER);
    RiCoordinateSystem(RI_CAMERA);
    BOOST_TEST((context().coordinateSystem("raster") == raster));
    BOOST_TEST((context().coordinateSystem("camera") == camera));
    RiEnd();
}

/**
 * RI's standard error handlers have the type RiErrorHandler takes, so a caller can pass one.
 **/
BOOST_AUTO_TEST_CASE(renderman_standard_error_handlers_can_be_passed_test) {
    RiErrorHandler(RiErrorIgnore);
    RiErrorHandler(RiErrorPrint);
    RiErrorHandler(RiErrorAbort);
    BOOST_TEST((std::is_same_v<RtErrorHandler, RtVoid(RtInt, RtInt, char*)>));
}

};  // namespace
