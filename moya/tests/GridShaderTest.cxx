/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Parameters.h>
#include <moya/libmoya/GridShader.h>
#include <moya/libmoya/RenderContext.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

typedef v3d::render::offline::rib::Declaration Declaration;
typedef v3d::render::offline::rib::ParameterList ParameterList;

/**
 * A context with a camera and nothing in it, ready for a shader and some lights.
 **/
void prepare(v3d::moya::RenderContext* rc) {
    rc->imageResolution(64, 48, 1.0f);
    rc->clipping(1.0f, 100.0f);
    rc->prepareWorld();
}

void add(ParameterList* list, const std::string & name, Declaration::Type type,
    const std::vector<float> & values) {
    list->add(name, Declaration(Declaration::Storage::UNIFORM, type, 1), values,
        std::vector<std::string>());
}

/**
 * A grid of four vertices, all at the same place and facing the same way, so that a case
 * asserting one vertex is asserting the shader rather than the interpolation.
 *
 * The camera looks down positive z, so a surface facing it has a normal along negative z.
 **/
v3d::moya::MicroPolygonGrid facing(const glm::vec3 & normal, const glm::vec3 & colour) {
    v3d::moya::MicroPolygonGrid grid(2);
    for (unsigned int i = 0; i < 2; i++) {
        for (unsigned int j = 0; j < 2; j++) {
            v3d::moya::Vertex vert;
            vert.point(glm::vec3(static_cast<float>(i), static_cast<float>(j), 5.0f));
            vert.normal(normal);
            vert.geometricNormal(normal);
            vert.color(colour);
            grid.addVertex(vert, i, j);
        }
    }
    return grid;
}

};  // namespace

/**
 * `matte` over a grid under one distant light is the cosine of
 * the angle between the surface and the light, at every vertex of the grid at once.
 *
 * distantlight aims from the shader's origin toward (0, 0, 1) by default, so the light
 * travels away from the camera and L - which points at the light - is along negative z.
 * A surface facing the camera faces the light head on.
 **/
BOOST_AUTO_TEST_CASE(moya_matte_under_one_light_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    rc.lightSource("distantlight", "key", ParameterList());
    rc.surface("matte", ParameterList());
    const v3d::moya::Shading shading = rc.shading();
    BOOST_REQUIRE_EQUAL(shading.lights.size(), 1u);

    v3d::moya::MicroPolygonGrid straight = facing(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f));
    rc.shader().shade(shading, &straight);
    for (unsigned int i = 0; i < 2; i++) {
        for (unsigned int j = 0; j < 2; j++) {
            BOOST_CHECK_CLOSE(straight.vertex(i, j).color().r, 1.0f, 0.1f);
        }
    }

    // sixty degrees off the light, whose cosine is a half
    v3d::moya::MicroPolygonGrid tilted =
        facing(glm::vec3(0.8660254f, 0.0f, -0.5f), glm::vec3(1.0f));
    rc.shader().shade(shading, &tilted);
    BOOST_CHECK_CLOSE(tilted.vertex(0, 0).color().r, 0.5f, 0.1f);

    /*
        A surface whose normal points away from the camera is shaded as though it did not.
        matte turns it with faceforward, so a polygon wound the other way is not simply
        black.
    */
    v3d::moya::MicroPolygonGrid backwards = facing(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f));
    rc.shader().shade(shading, &backwards);
    BOOST_CHECK_CLOSE(backwards.vertex(0, 0).color().r, 1.0f, 0.1f);
}

/**
 * A light behind the surface lights nothing. The illuminance cone of PI/2 inside
 * `diffuse` keeps it out of the sum, so a scene with lights on both sides does not come out
 * uniformly bright.
 **/
BOOST_AUTO_TEST_CASE(moya_a_light_behind_the_surface_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    ParameterList behind;
    // aimed back toward the camera, so it shines on the far side of a surface facing it
    add(&behind, "to", Declaration::Type::POINT, { 0.0f, 0.0f, -1.0f });
    rc.lightSource("distantlight", "back", behind);
    rc.surface("matte", ParameterList());

    v3d::moya::MicroPolygonGrid grid = facing(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f));
    rc.shader().shade(rc.shading(), &grid);
    BOOST_CHECK_SMALL(grid.vertex(0, 0).color().r, 0.0001f);
}

/**
 * `constant` is the shader that means no shading: it writes the colour dicing already
 * carried onto the grid. A scene that names no surface draws with it.
 **/
BOOST_AUTO_TEST_CASE(moya_constant_is_the_flat_colour_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    rc.surface("constant", ParameterList());

    v3d::moya::MicroPolygonGrid grid =
        facing(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.25f, 0.5f, 0.75f));
    rc.shader().shade(rc.shading(), &grid);
    BOOST_CHECK_CLOSE(grid.vertex(1, 1).color().r, 0.25f, 0.1f);
    BOOST_CHECK_CLOSE(grid.vertex(1, 1).color().g, 0.5f, 0.1f);
    BOOST_CHECK_CLOSE(grid.vertex(1, 1).color().b, 0.75f, 0.1f);

    // and a scene that names no surface at all draws the same thing
    v3d::moya::RenderContext bare;
    prepare(&bare);
    v3d::moya::MicroPolygonGrid unnamed =
        facing(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.25f, 0.5f, 0.75f));
    bare.shader().shade(bare.shading(), &unnamed);
    BOOST_CHECK_CLOSE(unnamed.vertex(1, 1).color().b, 0.75f, 0.1f);
}

/**
 * Two lights sum, and `Illuminate` turning one off changes the picture. The second is
 * green and half as bright, so which light each component came from is readable rather
 * than being one number that could have come from either.
 **/
BOOST_AUTO_TEST_CASE(moya_two_lights_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    rc.lightSource("distantlight", "key", ParameterList());

    ParameterList fill;
    // aimed along negative x, so L - which points at the light - is along positive x
    add(&fill, "to", Declaration::Type::POINT, { -1.0f, 0.0f, 0.0f });
    add(&fill, "intensity", Declaration::Type::FLOAT, { 0.5f });
    add(&fill, "lightcolor", Declaration::Type::COLOR, { 0.0f, 1.0f, 0.0f });
    rc.lightSource("distantlight", "fill", fill);
    rc.surface("matte", ParameterList());

    // halfway between the two, so each is at forty five degrees
    const glm::vec3 normal(0.70710678f, 0.0f, -0.70710678f);
    const float cosine = 0.70710678f;

    v3d::moya::MicroPolygonGrid both = facing(normal, glm::vec3(1.0f));
    rc.shader().shade(rc.shading(), &both);
    BOOST_CHECK_CLOSE(both.vertex(0, 0).color().r, cosine, 0.1f);
    BOOST_CHECK_CLOSE(both.vertex(0, 0).color().g, cosine + 0.5f * cosine, 0.1f);

    rc.illuminate("fill", false);
    v3d::moya::MicroPolygonGrid one = facing(normal, glm::vec3(1.0f));
    rc.shader().shade(rc.shading(), &one);
    BOOST_CHECK_CLOSE(one.vertex(0, 0).color().r, cosine, 0.1f);
    // the green the fill added is gone, so switching the light off changed the picture
    BOOST_CHECK_CLOSE(one.vertex(0, 0).color().g, cosine, 0.1f);

    rc.illuminate("fill", true);
    v3d::moya::MicroPolygonGrid back = facing(normal, glm::vec3(1.0f));
    rc.shader().shade(rc.shading(), &back);
    BOOST_CHECK_CLOSE(back.vertex(0, 0).color().g, cosine + 0.5f * cosine, 0.1f);
}

/**
 * Which lights are on is an attribute, so an `Illuminate` inside an AttributeBegin block
 * is local to it. The lights themselves belong to the frame and outlive the block, so a light
 * created inside a block can still be switched on after it.
 **/
BOOST_AUTO_TEST_CASE(moya_illuminate_is_an_attribute_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    rc.lightSource("distantlight", "key", ParameterList());
    rc.surface("matte", ParameterList());

    rc.attributeBegin();
    rc.illuminate("key", false);
    BOOST_CHECK_EQUAL(rc.shading().lights.size(), 0u);
    rc.attributeEnd();
    BOOST_CHECK_EQUAL(rc.shading().lights.size(), 1u);
}

/**
 * A light is where the scene put it. `pointlight` falls off with the square of the
 * distance and states its position in its own space, so a light the scene lifted lights a
 * surface below it from above rather than from the origin.
 **/
BOOST_AUTO_TEST_CASE(moya_a_light_is_placed_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    // two units in front of the surface, on the camera's side of it
    rc.translate(0.0f, 0.0f, 3.0f);
    rc.lightSource("pointlight", "bulb", ParameterList());
    rc.surface("matte", ParameterList());

    v3d::moya::MicroPolygonGrid grid = facing(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f));
    rc.shader().shade(rc.shading(), &grid);

    /*
        The vertex at (0, 0, 5) is two units from a light at (0, 0, 3), so L is (0, 0, -2):
        the cosine against the normal is one and the falloff is a quarter. A light left at
        the origin would be five units away and a twenty fifth as bright.
    */
    BOOST_CHECK_CLOSE(grid.vertex(0, 0).color().r, 0.25f, 1.0f);
}

/**
 * A light's own space is the light's while it runs, not the surface's it is lighting: a
 * light placed inside an attribute block the surface is outside of still lights from where
 * the block put it.
 **/
BOOST_AUTO_TEST_CASE(moya_a_light_runs_in_its_own_space_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    rc.attributeBegin();
    rc.translate(0.0f, 0.0f, 3.0f);
    rc.lightSource("pointlight", "bulb", ParameterList());
    rc.attributeEnd();
    // whether a light is on is an attribute, so the block's end switched it off again
    rc.illuminate("bulb", true);
    rc.surface("matte", ParameterList());

    v3d::moya::MicroPolygonGrid grid = facing(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f));
    rc.shader().shade(rc.shading(), &grid);

    // two units away as above; taking the surface's space for the light's would put the
    // light at the origin, five units away
    BOOST_CHECK_CLOSE(grid.vertex(0, 0).color().r, 0.25f, 1.0f);
}

/**
 * NDC is raster space divided by the resolution: x runs right and y runs down from the upper
 * left corner of the picture. A point on the top row of the picture has an NDC y of 0, and the
 * lower right corner is (1, 1).
 **/
BOOST_AUTO_TEST_CASE(moya_ndc_runs_down_from_the_top_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    glm::mat4x4 raster(1.0f);
    glm::mat4x4 ndc(1.0f);
    BOOST_REQUIRE(rc.shader().space("raster", &raster));
    BOOST_REQUIRE(rc.shader().space("NDC", &ndc));

    // a quarter of the way across the top row, back in camera space, where y is up
    const glm::vec4 top = glm::inverse(raster) * glm::vec4(16.0f, 0.0f, 1.0f, 1.0f);
    BOOST_CHECK_GT(top.y, 0.0f);
    const glm::vec4 high = ndc * top;
    BOOST_CHECK_CLOSE(high.x, 0.25f, 0.1f);
    BOOST_CHECK_SMALL(high.y, 0.0001f);

    const glm::vec4 corner = ndc * (glm::inverse(raster) * glm::vec4(64.0f, 48.0f, 1.0f, 1.0f));
    BOOST_CHECK_CLOSE(corner.x, 1.0f, 0.1f);
    BOOST_CHECK_CLOSE(corner.y, 1.0f, 0.1f);
}
