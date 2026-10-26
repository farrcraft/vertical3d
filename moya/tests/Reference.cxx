/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Reference.h"

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

void pixelCentres(v3d::moya::RenderContext & rc) {
    rc.sampling().samples = glm::uvec2(1, 1);
    rc.sampling().filter = v3d::render::offline::Filter::Box;
    rc.sampling().width = glm::vec2(1.0F, 1.0F);
}

v3d::moya::Vertex vertex(float x, float y, float z) {
    v3d::moya::Vertex v;
    v.point(glm::vec3(x, y, z));
    return v;
}

void scene(v3d::moya::RenderContext & rc) {
    rc.imageResolution({64, 48}, 1.0F);
    pixelCentres(rc);
    // the defaults are RI_EPSILON and RI_INFINITY, which leave the orthographic depth
    // scale at about 2e-38 and collapse every z onto the near plane
    rc.clipping({.hither = 1.0F, .yon = 100.0F});
    rc.prepareWorld();
    // the shader that means no shading, named rather than left as the default. The picture
    // shows that the shading language draws an unshaded surface unchanged
    rc.surface("constant", v3d::render::offline::rib::ParameterList());

    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.7F, -0.2F, 5.0F));
    polygon->addVertex(vertex(0.3F, -0.2F, 5.0F));
    polygon->addVertex(vertex(0.3F, 0.8F, 5.0F));
    polygon->addVertex(vertex(-0.7F, 0.8F, 5.0F));
    rc.addPolygon(polygon);
}

v3d::moya::Vertex shaded(float x, float y, float z, const glm::vec3 & normal) {
    v3d::moya::Vertex v;
    v.point(glm::vec3(x, y, z));
    v.normal(normal);
    return v;
}

void quad(v3d::moya::RenderContext & rc, float left, float right,
    const glm::vec3 & outer, const glm::vec3 & inner) {
    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(shaded(left, -0.8F, 5.0F, outer));
    polygon->addVertex(shaded(left, 0.8F, 5.0F, outer));
    polygon->addVertex(shaded(right, 0.8F, 5.0F, inner));
    polygon->addVertex(shaded(right, -0.8F, 5.0F, inner));
    rc.addPolygon(polygon);
}

void put(v3d::render::offline::rib::ParameterList* list, const std::string & name,
    v3d::render::offline::rib::Declaration::Type type, const std::vector<float> & values) {
    list->add(name, v3d::render::offline::rib::Declaration(
        v3d::render::offline::rib::Declaration::Storage::UNIFORM, type, 1), values,
        std::vector<std::string>());
}

void shadedScene(v3d::moya::RenderContext & rc) {
    rc.imageResolution({64, 48}, 1.0F);
    rc.clipping({.hither = 1.0F, .yon = 100.0F});
    rc.prepareWorld();

    typedef v3d::render::offline::rib::Declaration Declaration;
    v3d::render::offline::rib::ParameterList distant;
    put(&distant, "intensity", Declaration::Type::FLOAT, { 0.8F });
    put(&distant, "to", Declaration::Type::POINT, { 0.6F, -0.4F, 1.0F });
    rc.lightSource({.shader = "distantlight", .handle = "1"}, distant);

    v3d::render::offline::rib::ParameterList bulb;
    put(&bulb, "intensity", Declaration::Type::FLOAT, { 4.0F });
    rc.pushTransform();
    rc.translate(-1.0F, 1.0F, 2.0F);
    rc.lightSource({.shader = "pointlight", .handle = "2"}, bulb);
    rc.popTransform();

    const glm::vec3 tilted(-0.9F, 0.0F, -0.436F);
    const glm::vec3 square(0.0F, 0.0F, -1.0F);

    rc.attributeBegin();
    rc.color(glm::vec3(0.9F, 0.4F, 0.2F));
    rc.surface("matte", v3d::render::offline::rib::ParameterList());
    quad(rc, -1.2F, -0.1F, tilted, square);
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(0.3F, 0.5F, 0.9F));
    v3d::render::offline::rib::ParameterList plastic;
    put(&plastic, "roughness", Declaration::Type::FLOAT, { 0.08F });
    put(&plastic, "Ks", Declaration::Type::FLOAT, { 0.6F });
    rc.surface("plastic", plastic);
    quad(rc, 0.1F, 1.2F, square, glm::vec3(0.9F, 0.0F, -0.436F));
    rc.attributeEnd();
}

void focusScene(v3d::moya::RenderContext & rc, bool lens) {
    rc.imageResolution({64, 48}, 1.0F);
    rc.sampling().samples = glm::uvec2(4, 4);
    if (lens) {
        rc.sampling().fstop = 1.0F;
        rc.sampling().focalLength = 0.5F;
        rc.sampling().focalDistance = 4.0F;
    }
    rc.projection("perspective", 40.0F);
    rc.clipping({.hither = 0.1F, .yon = 100.0F});
    rc.prepareWorld();
    rc.surface("constant", v3d::render::offline::rib::ParameterList());

    rc.color(glm::vec3(0.9F, 0.2F, 0.2F));
    const boost::shared_ptr<v3d::moya::Polygon> near = boost::make_shared<v3d::moya::Polygon>();
    near->addVertex(vertex(-1.5F, -1.0F, 4.0F));
    near->addVertex(vertex(-0.2F, -1.0F, 4.0F));
    near->addVertex(vertex(-0.2F, 1.0F, 4.0F));
    near->addVertex(vertex(-1.5F, 1.0F, 4.0F));
    rc.addPolygon(near);

    rc.color(glm::vec3(0.2F, 0.4F, 0.9F));
    const boost::shared_ptr<v3d::moya::Polygon> far = boost::make_shared<v3d::moya::Polygon>();
    far->addVertex(vertex(0.5F, -2.5F, 10.0F));
    far->addVertex(vertex(4.0F, -2.5F, 10.0F));
    far->addVertex(vertex(4.0F, 2.5F, 10.0F));
    far->addVertex(vertex(0.5F, 2.5F, 10.0F));
    rc.addPolygon(far);
}

unsigned int partial(const v3d::render::offline::FrameBuffer & planes, unsigned int coverage, const Run & run) {
    unsigned int count = 0;
    for (unsigned int column = run.from; column <= run.to; column++) {
        const float value = planes.value(coverage, column, run.row);
        if (value > 0.02F && value < 0.98F) {
            count++;
        }
    }
    return count;
}

float largest(const v3d::render::offline::FrameBuffer & first, const v3d::render::offline::FrameBuffer & second,
    const Columns & columns) {
    float worst = 0.0F;
    for (unsigned int plane = 0; plane < first.planes(); plane++) {
        for (unsigned int row = 0; row < first.height(); row++) {
            for (unsigned int column = columns.from; column <= columns.to; column++) {
                worst = std::max(worst, std::fabs(first.value(plane, column, row) - second.value(plane, column, row)));
            }
        }
    }
    return worst;
}

void motionCamera(v3d::moya::RenderContext & rc) {
    rc.imageResolution({64, 48}, 1.0F);
    rc.sampling().samples = glm::uvec2(4, 4);
    rc.sampling().shutter = glm::vec2(0.0F, 1.0F);
    rc.clipping({.hither = 0.1F, .yon = 100.0F});
    rc.prepareWorld();
    rc.surface("constant", v3d::render::offline::rib::ParameterList());
}

void quadAt(v3d::moya::RenderContext & rc, const glm::vec3 (&corners)[4]) {
    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    for (const glm::vec3 & corner : corners) {
        polygon->addVertex(vertex(corner.x, corner.y, corner.z));
    }
    rc.addPolygon(polygon);
}

void motionScene(v3d::moya::RenderContext & rc) {
    motionCamera(rc);

    rc.attributeBegin();
    rc.motionBegin({ 0.0F, 1.0F });
    rc.translate(0.0F, 0.0F, 0.0F);
    rc.translate(0.5F, 0.0F, 0.0F);
    rc.motionEnd();
    rc.color(glm::vec3(0.9F, 0.8F, 0.2F));
    const glm::vec3 slid[4] = {
        glm::vec3(-1.2F, 0.1F, 5.0F), glm::vec3(-0.4F, 0.1F, 5.0F),
        glm::vec3(-0.4F, 0.8F, 5.0F), glm::vec3(-1.2F, 0.8F, 5.0F)
    };
    quadAt(rc, slid);
    rc.attributeEnd();

    rc.attributeBegin();
    rc.translate(0.6F, -0.4F, 5.0F);
    rc.motionBegin({ 0.0F, 1.0F });
    rc.rotate(0.0F, {0.0F, 0.0F, 1.0F});
    rc.rotate(90.0F, {0.0F, 0.0F, 1.0F});
    rc.motionEnd();
    rc.color(glm::vec3(0.2F, 0.7F, 0.9F));
    const glm::vec3 turned[4] = {
        glm::vec3(-0.35F, -0.15F, 0.0F), glm::vec3(0.35F, -0.15F, 0.0F),
        glm::vec3(0.35F, 0.15F, 0.0F), glm::vec3(-0.35F, 0.15F, 0.0F)
    };
    quadAt(rc, turned);
    rc.attributeEnd();
}

void checkRamp(const v3d::render::offline::FrameBuffer & planes, unsigned int coverage, unsigned int row) {
    BOOST_CHECK_EQUAL(planes.value(coverage, 7, row), 0.0F);
    for (unsigned int k = 0; k < 12; k++) {
        BOOST_TEST_CONTEXT("pixel " << 8 + k << " and " << 32 + k) {
            const float rising = (static_cast<float>(k) + 0.5F) / 12.0F;
            BOOST_CHECK_SMALL(planes.value(coverage, 8 + k, row) - rising, 0.1F);
            BOOST_CHECK_SMALL(planes.value(coverage, 32 + k, row) - (1.0F - rising), 0.1F);
        }
    }
    BOOST_CHECK_CLOSE(planes.value(coverage, 25, row), 1.0F, 1.0e-4F);
    BOOST_CHECK_EQUAL(planes.value(coverage, 44, row), 0.0F);
}

void check(const boost::shared_ptr<v3d::image::Image> & rendered, const char* reference,
    const char* output) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);
    const boost::shared_ptr<v3d::image::Image> committed = factory.read(reference);
    if (committed == nullptr) {
        boost::filesystem::create_directory("data_out");
        factory.write(output, rendered);
    }
    BOOST_REQUIRE_MESSAGE(committed != nullptr,
        std::string("cannot read the reference image ") + reference +
        " - what was rendered is in " + output);

    const v3d::image::Difference difference = v3d::image::compare(*rendered, *committed, 1);
    if (!difference.match) {
        boost::filesystem::create_directory("data_out");
        factory.write(output, rendered);
    }
    BOOST_CHECK_MESSAGE(difference.match,
        difference.description() + " - what was rendered instead is in " + output);
}

boost::shared_ptr<v3d::render::offline::FrameBuffer> read(const Scene & scene) {
    v3d::moya::RIBHandler handler;
    if (scene.hider != nullptr) {
        handler.context().hider(scene.hider);
    }
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(reader.read(scene.file, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");
    return handler.context().picture();
}

boost::shared_ptr<v3d::image::Image> picture(const v3d::moya::RenderContext & rc) {
    return rc.picture()->image(v3d::moya::Planes::CHANNELS);
}

void raytraceCamera(v3d::moya::RenderContext & rc) {
    rc.hider("raytrace");
    rc.imageResolution({64, 48}, 1.0F);
    rc.clipping({.hither = 0.001F, .yon = 100.0F});
    rc.projection("orthographic");
    rc.setTransform(glm::translate(glm::mat4x4(1.0F), glm::vec3(0.0F, 0.0F, 1.0F)));
}

void background(v3d::moya::RenderContext & rc, const glm::vec3 & colour) {
    v3d::render::offline::rib::ParameterList list;
    put(&list, "background", v3d::render::offline::rib::Declaration::Type::COLOR, { colour.r, colour.g, colour.b });
    rc.imager("background", list);
}

void triangleScene(v3d::moya::RenderContext & rc) {
    raytraceCamera(rc);
    pixelCentres(rc);
    background(rc, glm::vec3(0.15F, 0.25F, 0.45F));
    rc.prepareWorld();
    rc.surface("constant", v3d::render::offline::rib::ParameterList());
    rc.color(glm::vec3(0.9F, 0.2F, 0.2F));
    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.8F, -0.6F, 2.0F));
    polygon->addVertex(vertex(0.8F, -0.6F, 2.0F));
    polygon->addVertex(vertex(0.0F, 0.7F, 2.0F));
    rc.addPolygon(polygon);
}

void raytraceShadedScene(v3d::moya::RenderContext & rc) {
    typedef v3d::render::offline::rib::Declaration Declaration;
    raytraceCamera(rc);
    background(rc, glm::vec3(0.05F, 0.06F, 0.1F));
    rc.prepareWorld();

    v3d::render::offline::rib::ParameterList fill;
    put(&fill, "intensity", Declaration::Type::FLOAT, { 0.18F });
    rc.lightSource({.shader = "ambientlight", .handle = "0"}, fill);
    v3d::render::offline::rib::ParameterList distant;
    put(&distant, "intensity", Declaration::Type::FLOAT, { 0.9F });
    put(&distant, "to", Declaration::Type::POINT, { 0.7F, -0.7F, 1.0F });
    rc.lightSource({.shader = "distantlight", .handle = "1"}, distant);
    v3d::render::offline::rib::ParameterList lamp;
    put(&lamp, "intensity", Declaration::Type::FLOAT, { 1.2F });
    rc.pushTransform();
    rc.translate(-0.6F, 0.5F, 1.2F);
    rc.lightSource({.shader = "pointlight", .handle = "2"}, lamp);
    rc.popTransform();

    rc.attributeBegin();
    rc.color(glm::vec3(0.8F, 0.75F, 0.6F));
    rc.surface("matte", v3d::render::offline::rib::ParameterList());
    const glm::vec3 floor[4] = {
        glm::vec3(-1.15F, -1.2F, 3.0F), glm::vec3(1.15F, -1.2F, 3.0F),
        glm::vec3(1.15F, 1.2F, 3.0F), glm::vec3(-1.15F, 1.2F, 3.0F)
    };
    quadAt(rc, floor);
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(0.2F, 0.45F, 0.8F));
    v3d::render::offline::rib::ParameterList shiny;
    put(&shiny, "roughness", Declaration::Type::FLOAT, { 0.1F });
    put(&shiny, "Ks", Declaration::Type::FLOAT, { 0.5F });
    rc.surface("plastic", shiny);
    const glm::vec3 panel[4] = {
        glm::vec3(-0.5F, -0.45F, 2.2F), glm::vec3(0.35F, -0.45F, 2.2F),
        glm::vec3(0.35F, 0.4F, 2.2F), glm::vec3(-0.5F, 0.4F, 2.2F)
    };
    quadAt(rc, panel);
    rc.attributeEnd();
}

void spheresScene(v3d::moya::RenderContext & rc) {
    typedef v3d::render::offline::rib::Declaration Declaration;
    rc.hider("raytrace");
    rc.imageResolution({64, 48}, 1.0F);
    rc.clipping({.hither = 0.1F, .yon = 100.0F});
    rc.projection("perspective", 40.0F);
    // the floor's shader is a fixture beside the scene rather than one of the standard ones
    rc.searchpath("data:&");
    rc.prepareWorld();

    v3d::render::offline::rib::ParameterList fill;
    put(&fill, "intensity", Declaration::Type::FLOAT, { 0.2F });
    rc.lightSource({.shader = "ambientlight", .handle = "0"}, fill);
    v3d::render::offline::rib::ParameterList distant;
    put(&distant, "intensity", Declaration::Type::FLOAT, { 0.9F });
    put(&distant, "to", Declaration::Type::POINT, { 0.4F, -1.0F, 0.6F });
    rc.lightSource({.shader = "distantlight", .handle = "1"}, distant);

    rc.attributeBegin();
    rc.color(glm::vec3(0.85F, 0.8F, 0.7F));
    v3d::render::offline::rib::ParameterList checks;
    put(&checks, "size", Declaration::Type::FLOAT, { 0.5F });
    rc.surface("checked", checks);
    const glm::vec3 floor[4] = {
        glm::vec3(-4.0F, -1.0F, 2.0F), glm::vec3(4.0F, -1.0F, 2.0F),
        glm::vec3(4.0F, -1.0F, 12.0F), glm::vec3(-4.0F, -1.0F, 12.0F)
    };
    quadAt(rc, floor);
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(0.35F, 0.5F, 0.75F));
    rc.surface("matte", v3d::render::offline::rib::ParameterList());
    const glm::vec3 wall[4] = {
        glm::vec3(-4.0F, -1.0F, 12.0F), glm::vec3(4.0F, -1.0F, 12.0F),
        glm::vec3(4.0F, 4.0F, 12.0F), glm::vec3(-4.0F, 4.0F, 12.0F)
    };
    quadAt(rc, wall);
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(0.9F, 0.85F, 0.7F));
    v3d::render::offline::rib::ParameterList mirror;
    put(&mirror, "Ka", Declaration::Type::FLOAT, { 0.1F });
    put(&mirror, "Ks", Declaration::Type::FLOAT, { 0.6F });
    put(&mirror, "Kr", Declaration::Type::FLOAT, { 0.8F });
    rc.surface("shinymetal", mirror);
    rc.translate(-1.1F, -0.28F, 6.0F);
    BOOST_REQUIRE(rc.addSphere({.radius = 0.7F, .zmin = -0.7F, .zmax = 0.7F, .thetamax = 360.0F}));
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(1.0F));
    rc.opacity(glm::vec3(0.3F));
    rc.surface("glass", v3d::render::offline::rib::ParameterList());
    rc.translate(1.1F, -0.28F, 5.5F);
    BOOST_REQUIRE(rc.addSphere({.radius = 0.7F, .zmin = -0.7F, .zmax = 0.7F, .thetamax = 360.0F}));
    rc.attributeEnd();
}

void slidingQuad(v3d::moya::RenderContext & rc) {
    rc.imageResolution({64, 48}, 1.0F);
    rc.sampling().samples = glm::uvec2(8, 8);
    rc.sampling().filter = v3d::render::offline::Filter::Box;
    rc.sampling().width = glm::vec2(1.0F);
    rc.sampling().shutter = glm::vec2(0.0F, 1.0F);
    rc.clipping({.hither = 0.1F, .yon = 100.0F});
    rc.prepareWorld();
    rc.surface("constant", v3d::render::offline::rib::ParameterList());

    rc.motionBegin({ 0.0F, 1.0F });
    rc.translate(0.0F, 0.0F, 0.0F);
    rc.translate(0.5F, 0.0F, 0.0F);
    rc.motionEnd();
    const glm::vec3 corners[4] = {
        glm::vec3(-1.0F, -0.5F, 5.0F), glm::vec3(0.0F, -0.5F, 5.0F),
        glm::vec3(0.0F, 0.5F, 5.0F), glm::vec3(-1.0F, 0.5F, 5.0F)
    };
    quadAt(rc, corners);
}

};  // namespace v3d::moya::reference
