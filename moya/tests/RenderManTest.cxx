/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <moya/libmoya/RenderMan.h>
#include <moya/libmoya/RenderContext.h>

#include <string>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * The context the C entry points are landing in. RiGetContext is RI's own query for it, and
 * the handle it returns is the context.
 **/
v3d::moya::RenderContext & context() {
    return *static_cast<v3d::moya::RenderContext*>(RiGetContext());
}

};  // namespace

/**
 * The C interface and the RIB reader are the two ways into a render context, and neither
 * goes through the other: a va_list cannot be built at runtime, so a reader holding a
 * parsed parameter list could not call these.
 *
 * The shader requests therefore need a case on this path too. A parameter a C caller passes
 * is typed by what RiDeclare said, the same table a file's Declare fills, and it reaches the
 * same graphics state.
 **/
BOOST_AUTO_TEST_CASE(renderman_surface_and_lights_test) {
    RiBegin(RI_NULL);
    RiFormat(64, 48, 1.0f);
    RiClipping(1.0f, 100.0f);
    RiWorldBegin();

    RtFloat intensity = 0.5f;
    RtLightHandle key = RiLightSource(const_cast<char*>("distantlight"),
        const_cast<char*>("intensity"), &intensity, RI_NULL);
    BOOST_REQUIRE(key != nullptr);

    RtFloat roughness = 0.2f;
    RiSurface(const_cast<char*>("plastic"),
        const_cast<char*>("roughness"), &roughness, RI_NULL);

    v3d::moya::Shading shading = context().shading();
    BOOST_REQUIRE(shading.surface);
    BOOST_CHECK_EQUAL(shading.surface->name(), "plastic");
    BOOST_REQUIRE_EQUAL(shading.lights.size(), 1u);
    BOOST_CHECK_EQUAL(shading.lights[0].shader->name(), "distantlight");

    // the handle the interface gave back is the one that turns the light off again
    RiIlluminate(key, RI_FALSE);
    BOOST_CHECK_EQUAL(context().shading().lights.size(), 0u);

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

    RtFloat squish = 5.0f;
    RiSurface(const_cast<char*>("matte"), const_cast<char*>("squish"), &squish, RI_NULL);

    // matte declares no "squish", so the library dropped it and matte is still matte
    v3d::moya::Shading shading = context().shading();
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
    RiPixelSamples(4.0f, 4.0f);
    RiPixelFilter(RiCatmullRomFilter, 3.0f, 3.0f);
    RiDepthOfField(8.0f, 0.1f, 3.0f);
    RiShutter(0.0f, 0.5f);

    const v3d::render::offline::Sampling & sampling = context().sampling();
    BOOST_CHECK_EQUAL(sampling.samples.x, 4u);
    BOOST_CHECK_EQUAL(sampling.samples.y, 4u);
    BOOST_CHECK(sampling.filter == v3d::render::offline::Filter::CatmullRom);
    BOOST_CHECK_EQUAL(sampling.width.x, 3.0f);
    BOOST_CHECK_EQUAL(sampling.fstop, 8.0f);
    BOOST_CHECK_EQUAL(sampling.focalLength, 0.1f);
    BOOST_CHECK_EQUAL(sampling.focalDistance, 3.0f);
    BOOST_CHECK_EQUAL(sampling.shutter.x, 0.0f);
    BOOST_CHECK_EQUAL(sampling.shutter.y, 0.5f);

    RiPixelFilter(nullptr, 1.0f, 1.0f);
    BOOST_CHECK(context().sampling().filter == v3d::render::offline::Filter::CatmullRom);
    BOOST_CHECK_EQUAL(context().sampling().width.x, 3.0f);
    RiEnd();
}
