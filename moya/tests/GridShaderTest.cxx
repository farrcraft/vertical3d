/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/sl/runtime/Renderer.h>
#include <moya/libmoya/Polygon.h>
#include <moya/libmoya/Shading.h>
#include <moya/libmoya/Vertex.h>
#include <moya/libmoya/reyes/GridShader.h>
#include <moya/libmoya/RenderContext.h>
#include <moya/libmoya/reyes/MicroPolygonGrid.h>

#include <cmath>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
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
    rc->imageResolution({64, 48}, 1.0F);
    rc->clipping({.hither = 1.0F, .yon = 100.0F});
    rc->prepareWorld();
}

void add(ParameterList* list, const std::string & name, Declaration::Type type,
    const std::vector<float> & values) {
    list->add(name, Declaration(Declaration::Storage::UNIFORM, type, 1), values,
        std::vector<std::string>());
}

/**
 * Which way a grid faces, and its colour. The camera looks down positive z, so a surface
 * facing it has a normal along negative z.
 **/
struct Surface {
    glm::vec3 normal{0.0F, 0.0F, -1.0F};
    glm::vec3 colour{1.0F};
};

/**
 * A grid of four vertices, all facing the same way and with the same colour. A case that
 * checks one vertex then checks the shader and not the interpolation.
 **/
v3d::moya::reyes::MicroPolygonGrid facing(const Surface & surface) {
    const glm::vec3 & normal = surface.normal;
    const glm::vec3 & colour = surface.colour;
    v3d::moya::reyes::MicroPolygonGrid grid(2);
    for (unsigned int i = 0; i < 2; i++) {
        for (unsigned int j = 0; j < 2; j++) {
            v3d::moya::Vertex vert;
            vert.point(glm::vec3(static_cast<float>(i), static_cast<float>(j), 5.0F));
            vert.normal(normal);
            vert.geometricNormal(normal);
            vert.color(colour);
            grid.addVertex(vert, i, j);
        }
    }
    return grid;
}

/**
 * `matte` over a grid under one distant light is the cosine of the angle between the surface
 * and the light, at every vertex of the grid at once.
 *
 * distantlight aims from the shader's origin toward (0, 0, 1) by default, so the light travels
 * away from the camera. L points at the light, and is along negative z. A surface facing the
 * camera faces the light head on.
 **/
BOOST_AUTO_TEST_CASE(moya_matte_under_one_light_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    rc.lightSource({.shader = "distantlight", .handle = "key"}, ParameterList());
    rc.surface("matte", ParameterList());
    const v3d::moya::Shading shading = rc.shading();
    BOOST_REQUIRE_EQUAL(shading.lights.size(), 1U);

    v3d::moya::reyes::MicroPolygonGrid straight = facing({.normal = glm::vec3(0.0F, 0.0F, -1.0F), .colour = glm::vec3(1.0F)});
    rc.shader().shade(shading, &straight);
    for (unsigned int i = 0; i < 2; i++) {
        for (unsigned int j = 0; j < 2; j++) {
            BOOST_CHECK_CLOSE(straight.vertex(i, j).color().r, 1.0F, 0.1F);
        }
    }

    // sixty degrees off the light, whose cosine is a half
    v3d::moya::reyes::MicroPolygonGrid tilted =
        facing({.normal = glm::vec3(0.8660254F, 0.0F, -0.5F), .colour = glm::vec3(1.0F)});
    rc.shader().shade(shading, &tilted);
    BOOST_CHECK_CLOSE(tilted.vertex(0, 0).color().r, 0.5F, 0.1F);

    /*
        A surface whose normal points away from the camera is shaded as though it faced the
        camera. matte turns the normal with faceforward, so a polygon wound the other way is
        not black.
    */
    v3d::moya::reyes::MicroPolygonGrid backwards = facing({.normal = glm::vec3(0.0F, 0.0F, 1.0F), .colour = glm::vec3(1.0F)});
    rc.shader().shade(shading, &backwards);
    BOOST_CHECK_CLOSE(backwards.vertex(0, 0).color().r, 1.0F, 0.1F);
}

/**
 * A light behind the surface lights nothing. The illuminance cone of PI/2 inside `diffuse`
 * leaves it out of the sum.
 **/
BOOST_AUTO_TEST_CASE(moya_a_light_behind_the_surface_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    ParameterList behind;
    // aimed back toward the camera, so it shines on the far side of a surface facing it
    add(&behind, "to", Declaration::Type::POINT, { 0.0F, 0.0F, -1.0F });
    rc.lightSource({.shader = "distantlight", .handle = "back"}, behind);
    rc.surface("matte", ParameterList());

    v3d::moya::reyes::MicroPolygonGrid grid = facing({.normal = glm::vec3(0.0F, 0.0F, -1.0F), .colour = glm::vec3(1.0F)});
    rc.shader().shade(rc.shading(), &grid);
    BOOST_CHECK_SMALL(grid.vertex(0, 0).color().r, 0.0001F);
}

/**
 * `constant` is the shader that means no shading: it writes the colour dicing already
 * carried onto the grid. A scene that names no surface draws with it.
 **/
BOOST_AUTO_TEST_CASE(moya_constant_is_the_flat_colour_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    rc.surface("constant", ParameterList());

    v3d::moya::reyes::MicroPolygonGrid grid =
        facing({.normal = glm::vec3(0.0F, 0.0F, -1.0F), .colour = glm::vec3(0.25F, 0.5F, 0.75F)});
    rc.shader().shade(rc.shading(), &grid);
    BOOST_CHECK_CLOSE(grid.vertex(1, 1).color().r, 0.25F, 0.1F);
    BOOST_CHECK_CLOSE(grid.vertex(1, 1).color().g, 0.5F, 0.1F);
    BOOST_CHECK_CLOSE(grid.vertex(1, 1).color().b, 0.75F, 0.1F);

    // and a scene that names no surface at all draws the same thing
    v3d::moya::RenderContext bare;
    prepare(&bare);
    v3d::moya::reyes::MicroPolygonGrid unnamed =
        facing({.normal = glm::vec3(0.0F, 0.0F, -1.0F), .colour = glm::vec3(0.25F, 0.5F, 0.75F)});
    bare.shader().shade(bare.shading(), &unnamed);
    BOOST_CHECK_CLOSE(unnamed.vertex(1, 1).color().b, 0.75F, 0.1F);
}

/**
 * Two lights sum, and `Illuminate` turning one off changes the picture. The second light is
 * green and half as bright, so red comes from the first light alone and green from both.
 **/
BOOST_AUTO_TEST_CASE(moya_two_lights_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    rc.lightSource({.shader = "distantlight", .handle = "key"}, ParameterList());

    ParameterList fill;
    // aimed along negative x, so L, which points at the light, is along positive x
    add(&fill, "to", Declaration::Type::POINT, { -1.0F, 0.0F, 0.0F });
    add(&fill, "intensity", Declaration::Type::FLOAT, { 0.5F });
    add(&fill, "lightcolor", Declaration::Type::COLOR, { 0.0F, 1.0F, 0.0F });
    rc.lightSource({.shader = "distantlight", .handle = "fill"}, fill);
    rc.surface("matte", ParameterList());

    // halfway between the two, so each is at forty five degrees
    const glm::vec3 normal(0.70710678F, 0.0F, -0.70710678F);
    const float cosine = 0.70710678F;

    v3d::moya::reyes::MicroPolygonGrid both = facing({.normal = normal, .colour = glm::vec3(1.0F)});
    rc.shader().shade(rc.shading(), &both);
    BOOST_CHECK_CLOSE(both.vertex(0, 0).color().r, cosine, 0.1F);
    BOOST_CHECK_CLOSE(both.vertex(0, 0).color().g, cosine + (0.5F * cosine), 0.1F);

    rc.illuminate("fill", false);
    v3d::moya::reyes::MicroPolygonGrid one = facing({.normal = normal, .colour = glm::vec3(1.0F)});
    rc.shader().shade(rc.shading(), &one);
    BOOST_CHECK_CLOSE(one.vertex(0, 0).color().r, cosine, 0.1F);
    // the green the fill added is gone, so switching the light off changed the picture
    BOOST_CHECK_CLOSE(one.vertex(0, 0).color().g, cosine, 0.1F);

    rc.illuminate("fill", true);
    v3d::moya::reyes::MicroPolygonGrid back = facing({.normal = normal, .colour = glm::vec3(1.0F)});
    rc.shader().shade(rc.shading(), &back);
    BOOST_CHECK_CLOSE(back.vertex(0, 0).color().g, cosine + (0.5F * cosine), 0.1F);
}

/**
 * Which lights are on is an attribute, so an `Illuminate` inside an AttributeBegin block
 * is local to it.
 **/
BOOST_AUTO_TEST_CASE(moya_illuminate_is_an_attribute_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    rc.lightSource({.shader = "distantlight", .handle = "key"}, ParameterList());
    rc.surface("matte", ParameterList());

    rc.attributeBegin();
    rc.illuminate("key", false);
    BOOST_CHECK_EQUAL(rc.shading().lights.size(), 0U);
    rc.attributeEnd();
    BOOST_CHECK_EQUAL(rc.shading().lights.size(), 1U);
}

/**
 * A light is where the scene put it. `pointlight` falls off with the square of the distance,
 * and states its position in its own space. A light the scene moved lights a surface from
 * where it was moved to, not from the origin.
 **/
BOOST_AUTO_TEST_CASE(moya_a_light_is_placed_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    // two units in front of the surface, on the camera's side of it
    rc.translate(0.0F, 0.0F, 3.0F);
    rc.lightSource({.shader = "pointlight", .handle = "bulb"}, ParameterList());
    rc.surface("matte", ParameterList());

    v3d::moya::reyes::MicroPolygonGrid grid = facing({.normal = glm::vec3(0.0F, 0.0F, -1.0F), .colour = glm::vec3(1.0F)});
    rc.shader().shade(rc.shading(), &grid);

    /*
        The vertex at (0, 0, 5) is two units from a light at (0, 0, 3), so L is (0, 0, -2):
        the cosine against the normal is one and the falloff is a quarter. A light left at
        the origin would be five units away and a twenty fifth as bright.
    */
    BOOST_CHECK_CLOSE(grid.vertex(0, 0).color().r, 0.25F, 1.0F);
}

/**
 * A light runs in its own space, not in the space of the surface it is lighting. A light
 * placed inside an attribute block still lights from where the block put it, after the
 * block ends. The light outlives the block and can be switched on after it.
 **/
BOOST_AUTO_TEST_CASE(moya_a_light_runs_in_its_own_space_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    rc.attributeBegin();
    rc.translate(0.0F, 0.0F, 3.0F);
    rc.lightSource({.shader = "pointlight", .handle = "bulb"}, ParameterList());
    rc.attributeEnd();
    // whether a light is on is an attribute, so the block's end switched it off again
    rc.illuminate("bulb", true);
    rc.surface("matte", ParameterList());

    v3d::moya::reyes::MicroPolygonGrid grid = facing({.normal = glm::vec3(0.0F, 0.0F, -1.0F), .colour = glm::vec3(1.0F)});
    rc.shader().shade(rc.shading(), &grid);

    // two units away, as above. A light in the surface's space would sit at the origin, five
    // units away
    BOOST_CHECK_CLOSE(grid.vertex(0, 0).color().r, 0.25F, 1.0F);
}

/**
 * NDC is raster space divided by the resolution: x runs right and y runs down from the upper
 * left corner of the picture. A point on the top row of the picture has an NDC y of 0, and the
 * lower right corner is (1, 1).
 **/
BOOST_AUTO_TEST_CASE(moya_ndc_runs_down_from_the_top_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    glm::mat4x4 raster(1.0F);
    glm::mat4x4 ndc(1.0F);
    BOOST_REQUIRE(rc.shader().space("raster", &raster));
    BOOST_REQUIRE(rc.shader().space("NDC", &ndc));

    // a quarter of the way across the top row, back in camera space, where y is up
    const glm::vec4 top = glm::inverse(raster) * glm::vec4(16.0F, 0.0F, 1.0F, 1.0F);
    BOOST_CHECK_GT(top.y, 0.0F);
    const glm::vec4 high = ndc * top;
    BOOST_CHECK_CLOSE(high.x, 0.25F, 0.1F);
    BOOST_CHECK_SMALL(high.y, 0.0001F);

    const glm::vec4 corner = ndc * (glm::inverse(raster) * glm::vec4(64.0F, 48.0F, 1.0F, 1.0F));
    BOOST_CHECK_CLOSE(corner.x, 1.0F, 0.1F);
    BOOST_CHECK_CLOSE(corner.y, 1.0F, 0.1F);
}


/**
 * A grid of five vertices a side over the plane z = 5, facing the camera. Vertex (i, j) is at
 * x = j * 0.25 and y = i * 0.25, so u runs up y and v runs along x, and Du(P) ^ Dv(P) points
 * at the camera, along -z, as the normal does.
 **/
v3d::moya::reyes::MicroPolygonGrid plane() {
    v3d::moya::reyes::MicroPolygonGrid grid(5);
    for (unsigned int i = 0; i < 5; i++) {
        for (unsigned int j = 0; j < 5; j++) {
            v3d::moya::Vertex vert;
            vert.point(glm::vec3(static_cast<float>(j) * 0.25F, static_cast<float>(i) * 0.25F, 5.0F));
            vert.normal(glm::vec3(0.0F, 0.0F, -1.0F));
            vert.geometricNormal(glm::vec3(0.0F, 0.0F, -1.0F));
            vert.color(glm::vec3(1.0F));
            grid.addVertex(vert, i, j);
        }
    }
    return grid;
}

/**
 * The grid shader answers a derivative with the grid it is shading: its size a side, and the
 * step between two neighbours that it gives the shader as du and dv.
 **/
BOOST_AUTO_TEST_CASE(moya_the_grid_is_the_shape_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    rc.surface("constant", ParameterList());
    v3d::moya::reyes::MicroPolygonGrid grid = plane();
    rc.shader().shade(rc.shading(), &grid);

    const v3d::render::offline::sl::runtime::Renderer::Shape shape = rc.shader().shape();
    BOOST_CHECK(shape.layout == v3d::render::offline::sl::runtime::Renderer::Shape::Layout::GRID);
    BOOST_CHECK_EQUAL(shape.along, 5U);
    BOOST_CHECK_EQUAL(shape.across, 5U);
    BOOST_CHECK_CLOSE(shape.du, 0.25F, 0.001F);
    BOOST_CHECK_CLOSE(shape.dv, 0.25F, 0.001F);
}

/**
 * The normal of a plane bumped by a sine of s, against the same worked out by hand.
 *
 * s is u, which is i / 4. The bumped point is P + A sin(2 pi s) N, so its z is
 * 5 - A sin(2 pi s), and only along u does it change. Du(Pb) is (0, 1, dz), where dz is the
 * change in z to the next vertex, or from the previous one on the last row, times 4. Dv(Pb)
 * is (1, 0, 0). Their cross product is (0, dz, -1), which is normalised.
 **/
BOOST_AUTO_TEST_CASE(moya_a_bumped_plane_test) {
    const float amplitude = 0.1F;
    const float pi = 3.14159265F;
    const auto height = [&](unsigned int row) {
        return 5.0F - (amplitude * std::sin(2.0F * pi * static_cast<float>(row) / 4.0F));
    };

    v3d::moya::RenderContext rc;
    prepare(&rc);
    rc.searchpath("data:&");
    ParameterList bumped;
    add(&bumped, "amplitude", Declaration::Type::FLOAT, { amplitude });
    rc.surface("bumpnormal", bumped);
    v3d::moya::reyes::MicroPolygonGrid grid = plane();
    rc.shader().shade(rc.shading(), &grid);

    ParameterList still;
    add(&still, "amplitude", Declaration::Type::FLOAT, { 0.0F });
    rc.surface("bumpnormal", still);
    v3d::moya::reyes::MicroPolygonGrid flat = plane();
    rc.shader().shade(rc.shading(), &flat);

    for (const unsigned int row : { 0U, 1U, 4U }) {
        BOOST_TEST_CONTEXT("vertex " << row << ", 2") {
            const float dz = (row < 4 ? height(row + 1) - height(row) : height(row) - height(row - 1)) * 4.0F;
            const glm::vec3 expected = glm::normalize(glm::vec3(0.0F, dz, -1.0F));
            const glm::vec3 shaded = (grid.vertex(row, 2).color() * 2.0F) - glm::vec3(1.0F);
            BOOST_TEST(glm::length(shaded - expected) < 1.0e-3F);
            // unbumped, the plane's own normal
            const glm::vec3 plain = (flat.vertex(row, 2).color() * 2.0F) - glm::vec3(1.0F);
            BOOST_TEST(glm::length(plain - glm::vec3(0.0F, 0.0F, -1.0F)) < 1.0e-3F);
        }
    }
    // the sine rises fastest at s = 0, so the bump tilts the normal most there
    BOOST_TEST(std::fabs(((grid.vertex(0, 2).color() * 2.0F) - glm::vec3(1.0F)).y) > 0.2F);
}

/**
 * A quad light a hundredth of a unit across, sampled once, lights a grid as a point light at
 * the quad's centre does. Its one sample is within a two-hundredth of the centre.
 **/
BOOST_AUTO_TEST_CASE(moya_one_sample_of_a_small_area_light_is_a_point_light_test) {
    ParameterList bright;
    add(&bright, "intensity", Declaration::Type::FLOAT, { 3.0F });
    const glm::vec3 centre(0.5F, 0.5F, 3.0F);

    v3d::moya::RenderContext area;
    prepare(&area);
    area.attributeBegin();
    ParameterList once;
    once.add("nsamples", Declaration(Declaration::Storage::UNIFORM, Declaration::Type::INTEGER, 1), { 1.0F },
        std::vector<std::string>());
    area.attribute("light", once);
    area.areaLightSource({.shader = "arealight", .handle = "1"}, bright);
    const boost::shared_ptr<v3d::moya::Polygon> tiny = boost::make_shared<v3d::moya::Polygon>();
    for (const glm::vec2 & corner : { glm::vec2(-1.0F, -1.0F), glm::vec2(1.0F, -1.0F), glm::vec2(1.0F, 1.0F),
        glm::vec2(-1.0F, 1.0F) }) {
        v3d::moya::Vertex vert;
        vert.point(centre + glm::vec3(corner * 0.005F, 0.0F));
        tiny->addVertex(vert);
    }
    area.addPolygon(tiny);
    area.surface("matte", ParameterList());
    v3d::moya::reyes::MicroPolygonGrid lit = facing(Surface());
    area.shader().shade(area.shading(), &lit);

    v3d::moya::RenderContext point;
    prepare(&point);
    point.pushTransform();
    point.translate(centre.x, centre.y, centre.z);
    point.lightSource({.shader = "pointlight", .handle = "1"}, bright);
    point.popTransform();
    point.surface("matte", ParameterList());
    v3d::moya::reyes::MicroPolygonGrid pointLit = facing(Surface());
    point.shader().shade(point.shading(), &pointLit);

    for (unsigned int i = 0; i < 2; i++) {
        for (unsigned int j = 0; j < 2; j++) {
            BOOST_TEST_CONTEXT("vertex " << i << ", " << j) {
                BOOST_CHECK_GT(pointLit.vertex(i, j).color().r, 0.05F);
                BOOST_CHECK_CLOSE(lit.vertex(i, j).color().r, pointLit.vertex(i, j).color().r, 1.0F);
            }
        }
    }
}

/**
 * An area light's samples are seeded by the shading point, not by the order of shading. A grid
 * shaded twice, with another grid shaded between, comes out the same both times.
 **/
BOOST_AUTO_TEST_CASE(moya_an_area_light_samples_the_same_in_any_order_test) {
    v3d::moya::RenderContext rc;
    prepare(&rc);
    rc.areaLightSource({.shader = "arealight", .handle = "1"}, ParameterList());
    const boost::shared_ptr<v3d::moya::Polygon> panel = boost::make_shared<v3d::moya::Polygon>();
    for (const glm::vec3 & corner : { glm::vec3(-1.0F, -1.0F, 2.0F), glm::vec3(1.0F, -1.0F, 2.0F),
        glm::vec3(1.0F, 1.0F, 2.0F), glm::vec3(-1.0F, 1.0F, 2.0F) }) {
        v3d::moya::Vertex vert;
        vert.point(corner);
        panel->addVertex(vert);
    }
    rc.addPolygon(panel);
    rc.surface("matte", ParameterList());
    const v3d::moya::Shading shading = rc.shading();

    v3d::moya::reyes::MicroPolygonGrid first = plane();
    rc.shader().shade(shading, &first);
    // the panel lights the grid, which is all it has to light it
    BOOST_CHECK_GT(first.vertex(2, 2).color().r, 0.05F);
    v3d::moya::reyes::MicroPolygonGrid between = facing(Surface());
    rc.shader().shade(shading, &between);
    v3d::moya::reyes::MicroPolygonGrid again = plane();
    rc.shader().shade(shading, &again);
    for (unsigned int i = 0; i < 5; i++) {
        for (unsigned int j = 0; j < 5; j++) {
            BOOST_TEST_CONTEXT("vertex " << i << ", " << j) {
                BOOST_CHECK_EQUAL(first.vertex(i, j).color().r, again.vertex(i, j).color().r);
            }
        }
    }
}

};  // namespace
