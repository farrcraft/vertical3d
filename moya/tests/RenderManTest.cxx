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
 * The context the C entry points are landing in. RiGetContext is RI's own way to ask, and
 * the handle it answers with is the context.
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
 * That is exactly why the shader requests are worth a case on this path too. A parameter
 * a C caller passes is typed by what RiDeclare said, the same table a file's Declare
 * fills, and it reaches the same graphics state.
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
 * A parameter the scene declared itself is typed by that declaration, which is what makes
 * a shader parameter the standard has never heard of bindable at all.
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
