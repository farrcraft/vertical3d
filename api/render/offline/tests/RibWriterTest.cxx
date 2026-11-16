/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <api/render/offline/film/Sampling.h>
#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Handler.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/Reader.h>
#include <api/render/offline/rib/Recorder.h>
#include <api/render/offline/rib/Writer.h>

#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/glm.hpp>

namespace {

typedef v3d::render::offline::rib::Declaration Declaration;

/**
 * A list holding one parameter.
 **/
v3d::render::offline::rib::ParameterList one(const std::string & name, const Declaration & declaration,
    const std::vector<float> & values, const std::vector<std::string> & strings = {}) {
    v3d::render::offline::rib::ParameterList list;
    list.add(name, declaration, values, strings);
    return list;
}

const Declaration UNIFORM_FLOAT(Declaration::Storage::UNIFORM, Declaration::Type::FLOAT, 1);
const Declaration UNIFORM_STRING(Declaration::Storage::UNIFORM, Declaration::Type::STRING, 1);
const Declaration VERTEX_POINT(Declaration::Storage::VERTEX, Declaration::Type::POINT, 1);

/**
 * The quadrics but the sphere, points, a subdivision mesh with tags of two counts and of three,
 * an object and its instance, and a cone that deforms.
 **/
void geometry(v3d::render::offline::rib::Handler * to) {
    const v3d::render::offline::rib::ParameterList none;
    const std::vector<float> quad = { 0, 0, 5, 1, 0, 5, 1, 1, 5, 0, 1, 5 };
    to->objectBegin("leaf");
    to->points(4, one("P", VERTEX_POINT, quad));
    to->objectEnd();
    to->objectInstance("leaf");
    to->objectBegin("7");
    to->disk({.height = 0.5F, .radius = 2, .thetamax = 180}, none);
    to->objectEnd();
    to->objectInstance("7");
    to->motionBegin({ 0, 1 });
    to->cone({.height = 2, .radius = 0.5F, .thetamax = 300}, none);
    to->deformation()->cone({.height = 3, .radius = 0.5F, .thetamax = 300}, none);
    to->motionEnd();
    to->cylinder({.radius = 0.5F, .zmin = -2, .zmax = 2, .thetamax = 90}, none);
    to->hyperboloid({.point1 = { 1, 2, 3 }, .point2 = { -1, -2, 4 }, .thetamax = 360}, none);
    to->paraboloid({.rmax = 2, .zmin = 0.5F, .zmax = 1.5F, .thetamax = 120}, none);
    to->torus({.majorRadius = 3, .minorRadius = 0.5F, .phimin = 10, .phimax = 350, .thetamax = 270}, none);
    to->subdivisionMesh({.scheme = "catmull-clark", .counts = { 4 }, .indices = { 0, 1, 2, 3 }, .tags = { "crease", "interpolateboundary" },
        .arguments = { 2, 1, 1, 0 }, .integers = { 0, 1, 1 }, .floats = { 2.5F }}, one("P", VERTEX_POINT, quad));
    to->subdivisionMesh({.scheme = "loop", .counts = { 3 }, .indices = { 0, 1, 2 }, .tags = { "corner" }, .arguments = { 1, 1, 0 },
        .integers = { 2 }, .floats = { 4 }}, one("P", VERTEX_POINT, { 0, 0, 0, 1, 0, 0, 0, 1, 0 }));
}

/**
 * Every request the handler interface has, once each, with values that are not the defaults.
 **/
void everything(v3d::render::offline::rib::Handler * to) {
    const v3d::render::offline::rib::ParameterList none;
    const std::vector<float> quad = { 0, 0, 5, 1, 0, 5, 1, 1, 5, 0, 1, 5 };
    to->version(3.03F);
    to->declare({.name = "squish", .type = "uniform float"});
    to->option("limits", one("gridsize", Declaration(Declaration::Storage::UNIFORM, Declaration::Type::INTEGER, 1), { 18 }));
    to->hider("raytrace", none);
    to->format({.width = 320, .height = 240, .pixelAspect = 1.5F});
    to->frameAspectRatio(1.25F);
    to->screenWindow({.left = -2, .right = 2, .bottom = -1.5F, .top = 1.5F});
    to->cropWindow({.xmin = 0.1F, .xmax = 0.9F, .ymin = 0.2F, .ymax = 0.8F});
    to->projection("perspective", one("fov", UNIFORM_FLOAT, { 45 }));
    to->clipping({.hither = 0.1F, .yon = 1000});
    to->depthOfField({.fstop = 8, .focalLength = 0.1F, .focalDistance = 3});
    to->shutter({.open = 0, .close = 0.5F});
    to->pixelSamples(glm::uvec2(4, 2));
    to->pixelFilter(v3d::render::offline::film::Filter::CatmullRom, 3, 2);
    to->pixelVariance(0.01F);
    to->display({.name = "out.tif", .type = "file", .mode = "rgba"}, none);
    to->displayChannel({ .declaration = "varying color diffuse" }, one("source", UNIFORM_STRING, {}, { "color lpe:C<RD>L" }));
    to->sampleFilter({ .type = "PxrCryptomatte", .handle = "ids" }, one("layer", UNIFORM_STRING, {}, { "Material" }));
    to->displayFilter({ .type = "PxrBackgroundDisplayFilter", .handle = "plate" },
        one("backgroundColor", Declaration(Declaration::Storage::UNIFORM, Declaration::Type::COLOR, 1), { 0.1F, 0.2F, 0.3F }));
    to->frameBegin(7);
    to->transform(glm::mat4x4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));  // NOLINT(build/include_what_you_use)
    to->worldBegin();
    to->attributeBegin();
    to->attribute("identifier", one("name", UNIFORM_STRING, {}, { "a \"quoted\" \\ name" }));
    to->color(glm::vec3(0.9F, 0.2F, 0.1F));
    to->opacity(glm::vec3(0.5F));
    to->shadingRate(0.25F);
    to->sides(1);
    // a standard basis by name, and one of no name by its numbers
    to->basis({.u = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }, .ustep = 4,
        .v = { 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2 }, .vstep = 2});
    to->surface("plastic", one("squish", UNIFORM_FLOAT, { 5 }));
    to->displacement("lift", one("amount", UNIFORM_FLOAT, { 0.1F }));
    to->lightSource({.shader = "pointlight", .handle = "1"}, one("intensity", UNIFORM_FLOAT, { 2 }));
    to->areaLightSource({.shader = "arealight", .handle = "lamp"}, none);
    to->light({.shader = "PxrRectLight", .handle = "key"}, one("intensity", UNIFORM_FLOAT, { 3 }));
    to->pattern({ .type = "ND_constant_color3", .handle = "tint" },
        one("value", Declaration(Declaration::Storage::UNIFORM, Declaration::Type::COLOR, 1), { 0.9F, 0.6F, 0.2F }));
    to->bxdf({ .type = "ND_open_pbr_surface_surfaceshader", .handle = "gold" },
        one("base_color", Declaration(Declaration::Storage::UNIFORM, Declaration::Type::COLOR, 1, true), {}, { "tint:out" }));
    to->illuminate("1", false);
    to->illuminate("lamp", true);
    to->makeTexture({.picture = "a.png", .texture = "a.tx", .swrap = "periodic", .twrap = "clamp", .filter = "box", .swidth = 2, .twidth = 2}, none);
    to->imager("background", one("background", Declaration(Declaration::Storage::UNIFORM, Declaration::Type::COLOR, 1), { 0.1F, 0.2F, 0.3F }));
    to->transformBegin();
    to->identity();
    to->concatTransform(glm::mat4x4(2.0F));
    to->translate(1, 2, 3);
    to->rotate(90, glm::vec3(0, 1, 0));
    to->scale(2, 2, 2);
    to->coordinateSystem("lamp");
    to->coordSysTransform("lamp");
    to->transformEnd();
    to->motionBegin({ 0, 1 });
    to->polygon(4, one("P", VERTEX_POINT, quad));
    to->deformation()->polygon(4, one("P", VERTEX_POINT, quad));
    to->motionEnd();
    to->pointsPolygons({.counts = { 3, 3 }, .indices = { 0, 1, 2, 0, 2, 3 }}, one("P", VERTEX_POINT, quad));
    to->sphere({.radius = 0.5F, .zmin = -0.5F, .zmax = 0.5F, .thetamax = 270}, none);
    to->curves({.cubic = true, .vertices = { 4 }, .periodic = false}, one("P", VERTEX_POINT, quad));
    to->curves({.cubic = false, .vertices = { 2, 2 }, .periodic = true}, one("P", VERTEX_POINT, quad));
    geometry(to);
    to->attributeEnd();
    to->worldEnd();
    to->frameEnd();
}

std::string written(const std::string & source) {
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());
    std::istringstream in(source);
    std::ostringstream out;
    v3d::render::offline::rib::Writer writer(&out);
    BOOST_REQUIRE(reader.read(in, &writer));
    BOOST_TEST(reader.unrecognised().empty());
    BOOST_TEST(reader.unsupported().empty());
    return out.str();
}

/**
 * Every request the writer writes is read back as the same request: writing what the reader
 * read gives the same text again. The deforming polygon reaches the writer's deformation(),
 * which writes it as the polygon it is.
 **/
BOOST_AUTO_TEST_CASE(ribwriter_every_request_reads_back_test) {
    std::ostringstream first;
    v3d::render::offline::rib::Writer writer(&first);
    everything(&writer);

    BOOST_TEST(written(first.str()) == first.str());
}

/**
 * The geometry and instancing requests are written as RIB names them, and read back as the same
 * requests. A subdivision mesh's strings follow only when each tag gives three counts.
 **/
BOOST_AUTO_TEST_CASE(ribwriter_geometry_and_instancing_read_back_test) {
    std::ostringstream out;
    v3d::render::offline::rib::Writer writer(&out);
    geometry(&writer);

    const std::string expected =
        "ObjectBegin \"leaf\"\n"
        "Points \"P\" [ 0 0 5 1 0 5 1 1 5 0 1 5 ]\n"
        "ObjectEnd\n"
        "ObjectInstance \"leaf\"\n"
        "ObjectBegin 7\n"
        "Disk 0.5 2 180\n"
        "ObjectEnd\n"
        "ObjectInstance 7\n"
        "MotionBegin [ 0 1 ]\n"
        "Cone 2 0.5 300\n"
        "Cone 3 0.5 300\n"
        "MotionEnd\n"
        "Cylinder 0.5 -2 2 90\n"
        "Hyperboloid 1 2 3 -1 -2 4 360\n"
        "Paraboloid 2 0.5 1.5 120\n"
        "Torus 3 0.5 10 350 270\n"
        "SubdivisionMesh \"catmull-clark\" [ 4 ] [ 0 1 2 3 ] [ \"crease\" \"interpolateboundary\" ] [ 2 1 1 0 ] [ 0 1 1 ] [ 2.5 ]"
        " \"P\" [ 0 0 5 1 0 5 1 1 5 0 1 5 ]\n"
        "SubdivisionMesh \"loop\" [ 3 ] [ 0 1 2 ] [ \"corner\" ] [ 1 1 0 ] [ 2 ] [ 4 ] [ ] \"P\" [ 0 0 0 1 0 0 0 1 0 ]\n";
    BOOST_TEST(out.str() == expected);
    BOOST_TEST(written(out.str()) == out.str());
}

/**
 * A recording replays the geometry and instancing requests, a deforming quadric to the
 * deformation() of the handler replayed to.
 **/
BOOST_AUTO_TEST_CASE(ribrecorder_replays_geometry_and_instancing_test) {
    std::ostringstream direct;
    v3d::render::offline::rib::Writer writer(&direct);
    geometry(&writer);

    v3d::render::offline::rib::Recorder recording;
    geometry(&recording);
    std::ostringstream replayed;
    v3d::render::offline::rib::Writer again(&replayed);
    recording.replay(&again);
    BOOST_TEST(replayed.str() == direct.str());
}

/**
 * A number is written in the fewest digits that read back as the same float, so a value such
 * as 0.1 survives the trip whole.
 **/
BOOST_AUTO_TEST_CASE(ribwriter_numbers_read_back_exactly_test) {
    std::ostringstream out;
    v3d::render::offline::rib::Writer writer(&out);
    writer.shadingRate(0.1F);
    writer.translate(1.0F / 3.0F, 16777216.0F, -0.0F);

    BOOST_TEST(out.str() == "ShadingRate 0.1\nTranslate 0.33333334 16777216 -0\n");
}

/**
 * RIB has no number for an infinity or a NaN. An infinity becomes the largest float of its
 * sign and a NaN becomes 0, so the file still reads. DepthOfField's infinite fstop is written
 * as the request alone, which is a pinhole.
 **/
BOOST_AUTO_TEST_CASE(ribwriter_numbers_that_rib_cannot_hold_test) {
    std::ostringstream out;
    v3d::render::offline::rib::Writer writer(&out);
    writer.translate(std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN());
    writer.depthOfField({.fstop = std::numeric_limits<float>::infinity(), .focalLength = 0, .focalDistance = 0});

    BOOST_TEST(out.str() == "Translate 3.4028235e+38 -3.4028235e+38 0\nDepthOfField\n");
}

/**
 * A parameter the standard declares, or one declare() named, is written by its name. One typed
 * otherwise carries its declaration, so a reader types it the same way.
 **/
BOOST_AUTO_TEST_CASE(ribwriter_parameter_declarations_test) {
    std::ostringstream out;
    v3d::render::offline::rib::Writer writer(&out);
    writer.surface("plastic", one("Ks", UNIFORM_FLOAT, { 0.5F }));
    writer.surface("plastic", one("Ks", Declaration(Declaration::Storage::VARYING, Declaration::Type::FLOAT, 2), { 1, 2 }));
    writer.surface("plastic", one("squish", UNIFORM_FLOAT, { 5 }));
    writer.declare({.name = "squish", .type = "float"});
    writer.surface("plastic", one("squish", UNIFORM_FLOAT, { 5 }));

    BOOST_TEST(out.str() ==
        "Surface \"plastic\" \"Ks\" [ 0.5 ]\n"
        "Surface \"plastic\" \"varying float[2] Ks\" [ 1 2 ]\n"
        "Surface \"plastic\" \"uniform float squish\" [ 5 ]\n"
        "Declare \"squish\" \"float\"\n"
        "Surface \"plastic\" \"squish\" [ 5 ]\n");
}

/**
 * A light handle that is a number is written as one, as RIB 3.03 writes it. Any other handle,
 * one with a leading zero among them, is a string.
 **/
BOOST_AUTO_TEST_CASE(ribwriter_light_handles_test) {
    std::ostringstream out;
    v3d::render::offline::rib::Writer writer(&out);
    const v3d::render::offline::rib::ParameterList none;
    writer.lightSource({.shader = "pointlight", .handle = "12"}, none);
    writer.lightSource({.shader = "pointlight", .handle = "012"}, none);
    writer.lightSource({.shader = "pointlight", .handle = "1-2"}, none);

    BOOST_TEST(out.str() ==
        "LightSource \"pointlight\" 12\n"
        "LightSource \"pointlight\" \"012\"\n"
        "LightSource \"pointlight\" \"1-2\"\n");
}

/**
 * A recording replays every request it kept, in order, so replaying it into a writer writes
 * what writing directly writes.
 **/
BOOST_AUTO_TEST_CASE(ribrecorder_replays_every_request_test) {
    std::ostringstream direct;
    v3d::render::offline::rib::Writer writer(&direct);
    everything(&writer);

    v3d::render::offline::rib::Recorder recording;
    BOOST_TEST(recording.empty());
    everything(&recording);
    BOOST_TEST(!recording.empty());
    std::ostringstream replayed;
    v3d::render::offline::rib::Writer again(&replayed);
    recording.replay(&again);

    BOOST_TEST(replayed.str() == direct.str());
}

/**
 * A recording keeps copies, so it replays the same requests after what it was given is gone,
 * and as often as it is replayed.
 **/
BOOST_AUTO_TEST_CASE(ribrecorder_keeps_copies_test) {
    v3d::render::offline::rib::Recorder recording;
    {
        std::string name = "plastic";
        v3d::render::offline::rib::ParameterList list = one("Ks", UNIFORM_FLOAT, { 0.5F });
        recording.surface(name, list);
        name = "matte";
        list = v3d::render::offline::rib::ParameterList();
    }
    std::ostringstream first;
    v3d::render::offline::rib::Writer writer(&first);
    recording.replay(&writer);
    recording.replay(&writer);

    BOOST_TEST(first.str() == "Surface \"plastic\" \"Ks\" [ 0.5 ]\nSurface \"plastic\" \"Ks\" [ 0.5 ]\n");
}

/**
 * A deforming primitive is replayed to the deformation() of the handler replayed to, and is
 * dropped when that is null, as a reader drops it.
 **/
BOOST_AUTO_TEST_CASE(ribrecorder_replays_a_deformation_test) {
    class Primitives final : public v3d::render::offline::rib::Handler {
     public:
        void polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) override {
            (void)vertices;
            (void)parameters;
            polygons_++;
        }
        unsigned int polygons_ = 0;
    };
    v3d::render::offline::rib::Recorder recording;
    const v3d::render::offline::rib::ParameterList none;
    recording.polygon(3, none);
    recording.deformation()->polygon(3, none);

    Primitives rigid;
    recording.replay(&rigid);
    BOOST_TEST(rigid.polygons_ == 1U);

    std::ostringstream out;
    v3d::render::offline::rib::Writer writer(&out);
    recording.replay(&writer);
    BOOST_TEST(out.str() == "Polygon\nPolygon\n");
}

};  // namespace
