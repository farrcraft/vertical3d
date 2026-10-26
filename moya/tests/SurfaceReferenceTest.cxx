/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Reader.h>
#include <moya/libmoya/Planes.h>
#include <moya/libmoya/Polygon.h>
#include <moya/libmoya/RIBHandler.h>
#include <moya/libmoya/RenderContext.h>
#include <moya/libmoya/Renderer.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/operations.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Reference.h"

namespace v3d::moya::reference {
namespace {

/*
    The same scene as data/reference-bump.rib, and as data/raytrace-bump.rib under the ray
    hider. A plane faces the camera, bumped by noise in its own space and lit low across its
    face, so the bumps show as light and shade. A plane with no bumps would be one flat colour.
*/
void bumpScene(v3d::moya::RenderContext & rc, const char* hider) {
    rc.hider(hider);
    rc.imageResolution({64, 48}, 1.0F);
    pixelCentres(rc);
    rc.clipping({.hither = 1.0F, .yon = 100.0F});
    rc.searchpath("data:&");
    rc.prepareWorld();

    typedef v3d::render::offline::rib::Declaration Declaration;
    v3d::render::offline::rib::ParameterList ambient;
    put(&ambient, "intensity", Declaration::Type::FLOAT, { 0.1F });
    rc.lightSource({.shader = "ambientlight", .handle = "0"}, ambient);
    v3d::render::offline::rib::ParameterList low;
    put(&low, "intensity", Declaration::Type::FLOAT, { 1.0F });
    put(&low, "to", Declaration::Type::POINT, { 0.8F, -0.3F, 0.6F });
    rc.lightSource({.shader = "distantlight", .handle = "1"}, low);

    rc.color(glm::vec3(0.85F, 0.8F, 0.7F));
    rc.surface("bumpy", v3d::render::offline::rib::ParameterList());
    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-1.2F, -0.9F, 5.0F));
    polygon->addVertex(vertex(-1.2F, 0.9F, 5.0F));
    polygon->addVertex(vertex(1.2F, 0.9F, 5.0F));
    polygon->addVertex(vertex(1.2F, -0.9F, 5.0F));
    rc.addPolygon(polygon);
}

/**
 * A bumped plane under the reyes hider. calculatenormal differences the bumped points across
 * each grid, so the shading follows the bumps.
 **/
BOOST_AUTO_TEST_CASE(moya_bump_reference_test) {
    v3d::moya::RenderContext rc;
    bumpScene(rc, "hidden");
    rc.render();

    check(rc.picture()->image(v3d::moya::Planes::CHANNELS), BUMP, BUMP_RENDERED);
}

/**
 * The same bumped plane said in RIB, against the same committed picture.
 **/
BOOST_AUTO_TEST_CASE(moya_bump_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(BUMP_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().picture()->image(v3d::moya::Planes::CHANNELS),
        BUMP, BUMP_RIB_RENDERED);
}

/**
 * The bumped plane under the ray hider. Each hit is shaded with two neighbours a pixel away,
 * so calculatenormal follows the bumps here too. The hiders sample differently, so this
 * picture is not compared with reference-bump.png.
 **/
BOOST_AUTO_TEST_CASE(moya_traced_bump_reference_test) {
    v3d::moya::RenderContext rc;
    bumpScene(rc, "raytrace");
    rc.render();

    check(rc.picture()->image(v3d::moya::Planes::CHANNELS), TRACED_BUMP, TRACED_BUMP_RENDERED);
}

/**
 * The same traced bumped plane said in RIB, against the same committed picture.
 **/
BOOST_AUTO_TEST_CASE(moya_traced_bump_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(TRACED_BUMP_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().picture()->image(v3d::moya::Planes::CHANNELS),
        TRACED_BUMP, TRACED_BUMP_RIB_RENDERED);
}

/*
    The same scene as data/reference-area.rib, and as data/raytrace-area.rib under the ray
    hider. A box stands in front of a wall, lit by a quad light up and to its left. The light
    is sampled at 16 points for every shading point, so the box's shadow on the wall is soft.
    The light's quad is drawn too, in white.
*/
void areaScene(v3d::moya::RenderContext & rc, const char* hider) {
    rc.hider(hider);
    rc.imageResolution({64, 48}, 1.0F);
    pixelCentres(rc);
    rc.screenWindow({.left = -2.0F, .right = 2.0F, .bottom = -1.5F, .top = 1.5F});
    rc.clipping({.hither = 0.1F, .yon = 100.0F});
    rc.prepareWorld();

    typedef v3d::render::offline::rib::Declaration Declaration;
    v3d::render::offline::rib::ParameterList ambient;
    put(&ambient, "intensity", Declaration::Type::FLOAT, { 0.08F });
    rc.lightSource({.shader = "ambientlight", .handle = "0"}, ambient);

    rc.attributeBegin();
    v3d::render::offline::rib::ParameterList count;
    put(&count, "nsamples", Declaration::Type::INTEGER, { 16.0F });
    rc.attribute("light", count);
    v3d::render::offline::rib::ParameterList bright;
    put(&bright, "intensity", Declaration::Type::FLOAT, { 6.0F });
    rc.areaLightSource({.shader = "arealight", .handle = "1"}, bright);
    rc.surface("constant", v3d::render::offline::rib::ParameterList());
    quadAt(rc, {glm::vec3(-1.9F, 0.6F, 3.0F), glm::vec3(-0.9F, 0.6F, 3.0F), glm::vec3(-0.9F, 1.4F, 3.0F),
        glm::vec3(-1.9F, 1.4F, 3.0F)});
    rc.attributeEnd();
    // the block switched the light off, as it does any light made in one
    rc.illuminate("1", true);

    rc.attributeBegin();
    rc.color(glm::vec3(0.8F, 0.8F, 0.75F));
    rc.surface("matte", v3d::render::offline::rib::ParameterList());
    quadAt(rc, {glm::vec3(-2.0F, -1.5F, 6.0F), glm::vec3(-2.0F, 1.5F, 6.0F), glm::vec3(2.0F, 1.5F, 6.0F),
        glm::vec3(2.0F, -1.5F, 6.0F)});
    rc.attributeEnd();

    // the box from (-0.2, -0.6, 4.6) to (0.6, 0.2, 5.4), each face wound to face out
    rc.attributeBegin();
    rc.color(glm::vec3(0.7F, 0.3F, 0.25F));
    rc.surface("matte", v3d::render::offline::rib::ParameterList());
    const float x0 = -0.2F;
    const float x1 = 0.6F;
    const float y0 = -0.6F;
    const float y1 = 0.2F;
    const float z0 = 4.6F;
    const float z1 = 5.4F;
    quadAt(rc, {glm::vec3(x0, y0, z0), glm::vec3(x0, y1, z0), glm::vec3(x1, y1, z0), glm::vec3(x1, y0, z0)});
    quadAt(rc, {glm::vec3(x1, y0, z1), glm::vec3(x1, y1, z1), glm::vec3(x0, y1, z1), glm::vec3(x0, y0, z1)});
    quadAt(rc, {glm::vec3(x0, y0, z1), glm::vec3(x0, y1, z1), glm::vec3(x0, y1, z0), glm::vec3(x0, y0, z0)});
    quadAt(rc, {glm::vec3(x1, y0, z0), glm::vec3(x1, y1, z0), glm::vec3(x1, y1, z1), glm::vec3(x1, y0, z1)});
    quadAt(rc, {glm::vec3(x0, y1, z0), glm::vec3(x0, y1, z1), glm::vec3(x1, y1, z1), glm::vec3(x1, y1, z0)});
    quadAt(rc, {glm::vec3(x0, y0, z1), glm::vec3(x0, y0, z0), glm::vec3(x1, y0, z0), glm::vec3(x1, y0, z1)});
    rc.attributeEnd();
}

/**
 * A soft shadow from an area light under the reyes hider. Each grid vertex runs the light
 * at 16 points on its quad, each with its own shadow ray, so the shadow's edge is a gradient.
 **/
BOOST_AUTO_TEST_CASE(moya_area_reference_test) {
    v3d::moya::RenderContext rc;
    areaScene(rc, "hidden");
    rc.render();

    check(rc.picture()->image(v3d::moya::Planes::CHANNELS), AREA, AREA_RENDERED);
}

/**
 * The same area light said in RIB, against the same committed picture.
 **/
BOOST_AUTO_TEST_CASE(moya_area_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(AREA_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().picture()->image(v3d::moya::Planes::CHANNELS), AREA, AREA_RIB_RENDERED);
}

/**
 * The soft shadow under the ray hider. It is not compared with reference-area.png, because the
 * hiders sample differently.
 **/
BOOST_AUTO_TEST_CASE(moya_traced_area_reference_test) {
    v3d::moya::RenderContext rc;
    areaScene(rc, "raytrace");
    rc.render();

    check(rc.picture()->image(v3d::moya::Planes::CHANNELS), TRACED_AREA, TRACED_AREA_RENDERED);
}

/**
 * The traced area light said in RIB, against the same committed picture.
 **/
BOOST_AUTO_TEST_CASE(moya_traced_area_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(TRACED_AREA_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().picture()->image(v3d::moya::Planes::CHANNELS),
        TRACED_AREA, TRACED_AREA_RIB_RENDERED);
}

/*
    The same scene as data/reference-glass.rib. A blue quad of opacity one half stands in front
    of a checked plane, which is tilted away from the camera so its checks run across both x
    and z. The checks show through the quad, darkened by half and tinted.
*/
void glassScene(v3d::moya::RenderContext & rc) {
    rc.imageResolution({64, 48}, 1.0F);
    pixelCentres(rc);
    rc.clipping({.hither = 1.0F, .yon = 100.0F});
    rc.searchpath("data:&");
    rc.prepareWorld();

    typedef v3d::render::offline::rib::Declaration Declaration;
    v3d::render::offline::rib::ParameterList ambient;
    put(&ambient, "intensity", Declaration::Type::FLOAT, { 0.3F });
    rc.lightSource({.shader = "ambientlight", .handle = "0"}, ambient);
    v3d::render::offline::rib::ParameterList ahead;
    put(&ahead, "intensity", Declaration::Type::FLOAT, { 0.8F });
    put(&ahead, "to", Declaration::Type::POINT, { 0.0F, 0.0F, 1.0F });
    rc.lightSource({.shader = "distantlight", .handle = "1"}, ahead);

    rc.attributeBegin();
    rc.color(glm::vec3(0.9F, 0.85F, 0.7F));
    v3d::render::offline::rib::ParameterList checks;
    put(&checks, "size", Declaration::Type::FLOAT, { 0.4F });
    rc.surface("checked", checks);
    quadAt(rc, {glm::vec3(-1.4F, -1.1F, 4.0F), glm::vec3(-1.4F, 1.1F, 8.0F), glm::vec3(1.4F, 1.1F, 8.0F),
        glm::vec3(1.4F, -1.1F, 4.0F)});
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(0.2F, 0.5F, 0.9F));
    rc.opacity(glm::vec3(0.5F));
    rc.surface("constant", v3d::render::offline::rib::ParameterList());
    quadAt(rc, {glm::vec3(-0.6F, -0.5F, 3.0F), glm::vec3(-0.6F, 0.5F, 3.0F), glm::vec3(0.6F, 0.5F, 3.0F),
        glm::vec3(0.6F, -0.5F, 3.0F)});
    rc.attributeEnd();
}

/**
 * A translucent surface under the reyes hider. Each sample keeps the quad and the plane behind
 * it, and composites them front to back once the buckets are done.
 **/
BOOST_AUTO_TEST_CASE(moya_glass_reference_test) {
    v3d::moya::RenderContext rc;
    glassScene(rc);
    rc.render();

    check(rc.picture()->image(v3d::moya::Planes::CHANNELS), GLASS, GLASS_RENDERED);
}

/**
 * The same translucent quad said in RIB, against the same committed picture.
 **/
BOOST_AUTO_TEST_CASE(moya_glass_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(GLASS_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().picture()->image(v3d::moya::Planes::CHANNELS), GLASS, GLASS_RIB_RENDERED);
}

/*
    The same scene as data/reference-displaced.rib, and as data/raytrace-displaced.rib under the
    ray hider. A plane is rippled three times across x by the lift displacement shader, toward
    the camera, and lit from the left. The plane's normal points away from the camera, so the
    amount is negative.
*/
void displacedScene(v3d::moya::RenderContext & rc, const char* hider) {
    rc.hider(hider);
    rc.imageResolution({64, 48}, 1.0F);
    pixelCentres(rc);
    rc.clipping({.hither = 1.0F, .yon = 100.0F});
    rc.searchpath("data:&");
    rc.prepareWorld();

    typedef v3d::render::offline::rib::Declaration Declaration;
    v3d::render::offline::rib::ParameterList ambient;
    put(&ambient, "intensity", Declaration::Type::FLOAT, { 0.1F });
    rc.lightSource({.shader = "ambientlight", .handle = "0"}, ambient);
    v3d::render::offline::rib::ParameterList side;
    put(&side, "intensity", Declaration::Type::FLOAT, { 1.0F });
    put(&side, "to", Declaration::Type::POINT, { 0.8F, 0.0F, 1.0F });
    rc.lightSource({.shader = "distantlight", .handle = "1"}, side);

    rc.color(glm::vec3(0.8F, 0.8F, 0.85F));
    rc.surface("matte", v3d::render::offline::rib::ParameterList());
    v3d::render::offline::rib::ParameterList bound;
    put(&bound, "sphere", Declaration::Type::FLOAT, { 0.16F });
    rc.attribute("displacementbound", bound);
    v3d::render::offline::rib::ParameterList ripples;
    put(&ripples, "amount", Declaration::Type::FLOAT, { -0.08F });
    put(&ripples, "wave", Declaration::Type::FLOAT, { 0.08F });
    put(&ripples, "frequency", Declaration::Type::FLOAT, { 7.853982F });
    rc.displacement("lift", ripples);
    quadAt(rc, {glm::vec3(-1.2F, -0.9F, 5.0F), glm::vec3(1.2F, -0.9F, 5.0F), glm::vec3(1.2F, 0.9F, 5.0F),
        glm::vec3(-1.2F, 0.9F, 5.0F)});
}

/**
 * A displaced surface under the reyes hider. Each grid is moved by the displacement shader
 * before the surface shader runs, so matte shades the moved normals. The ripples show as light
 * and shade across a plane that is flat in the scene.
 **/
BOOST_AUTO_TEST_CASE(moya_displaced_reference_test) {
    v3d::moya::RenderContext rc;
    displacedScene(rc, "hidden");
    rc.render();

    check(rc.picture()->image(v3d::moya::Planes::CHANNELS), DISPLACED, DISPLACED_RENDERED);
}

/**
 * The same displaced plane said in RIB, against the same committed picture.
 **/
BOOST_AUTO_TEST_CASE(moya_displaced_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(DISPLACED_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().picture()->image(v3d::moya::Planes::CHANNELS),
        DISPLACED, DISPLACED_RIB_RENDERED);
}

/**
 * The displaced plane under the ray hider. The plane is diced and displaced into the traced
 * scene, so the rays meet the rippled surface and its silhouette. It is not compared with
 * reference-displaced.png, because the hiders sample differently.
 **/
BOOST_AUTO_TEST_CASE(moya_traced_displaced_reference_test) {
    v3d::moya::RenderContext rc;
    displacedScene(rc, "raytrace");
    rc.render();

    check(rc.picture()->image(v3d::moya::Planes::CHANNELS), TRACED_DISPLACED, TRACED_DISPLACED_RENDERED);
}

/**
 * The same traced displaced plane said in RIB, against the same committed picture.
 **/
BOOST_AUTO_TEST_CASE(moya_traced_displaced_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(TRACED_DISPLACED_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().picture()->image(v3d::moya::Planes::CHANNELS),
        TRACED_DISPLACED, TRACED_DISPLACED_RIB_RENDERED);
}

};  // namespace
};  // namespace v3d::moya::reference
