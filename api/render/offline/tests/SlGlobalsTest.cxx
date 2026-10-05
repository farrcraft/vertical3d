/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/sl/Globals.h>
#include <api/render/offline/sl/ShaderLibrary.h>

#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

namespace {

typedef v3d::render::offline::rib::Declaration Declaration;
typedef v3d::render::offline::rib::ParameterList ParameterList;
typedef v3d::render::offline::sl::Globals Globals;
typedef v3d::render::offline::sl::InstancePtr InstancePtr;
typedef v3d::render::offline::sl::ShaderLibrary ShaderLibrary;
typedef v3d::render::offline::sl::ShaderType ShaderType;
typedef v3d::render::offline::sl::runtime::Machine Machine;
typedef v3d::render::offline::sl::runtime::Value Value;

ShaderLibrary & library() {
    static ShaderLibrary shaders(boost::make_shared<v3d::log::Logger>());
    return shaders;
}

};  // namespace

/**
 * A surface reads the point a hider bound into each lane, and what it leaves in Ci and Oi
 * comes back per lane - `constant` is Os * Cs and Os.
 **/
BOOST_AUTO_TEST_CASE(slglobals_surface_test) {
    const InstancePtr shader = library().instance("constant", ShaderType::SURFACE, ParameterList());
    BOOST_REQUIRE(shader);
    Machine machine;
    machine.prepare(shader->program(), 2);
    const Globals globals(shader->program());

    v3d::render::offline::sl::Point point;
    point.colour = glm::vec3(0.5f, 0.5f, 1.0f);
    point.opacity = glm::vec3(0.5f);
    globals.surface(&machine, 0, point);
    point.colour = glm::vec3(1.0f, 0.0f, 0.0f);
    point.opacity = glm::vec3(1.0f);
    globals.surface(&machine, 1, point);
    shader->write(&machine);
    BOOST_REQUIRE(machine.run(shader->program()));

    const glm::vec3 first = globals.colour(machine, 0, glm::vec3(-1.0f));
    BOOST_CHECK_CLOSE(first.r, 0.25f, 0.01f);
    BOOST_CHECK_CLOSE(first.b, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(globals.opacity(machine, 0, glm::vec3(-1.0f)).g, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(globals.colour(machine, 1, glm::vec3(-1.0f)).r, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(globals.opacity(machine, 1, glm::vec3(-1.0f)).r, 1.0f, 0.01f);
}

/**
 * A program a renderer was not given has no globals at all, and what is read back from it
 * is whatever the renderer said otherwise.
 **/
BOOST_AUTO_TEST_CASE(slglobals_absent_test) {
    const Globals globals;
    Machine machine;
    BOOST_CHECK_CLOSE(globals.colour(machine, 0, glm::vec3(0.3f)).r, 0.3f, 0.01f);
    BOOST_CHECK_CLOSE(globals.alpha(machine, 0, 0.7f), 0.7f, 0.01f);
}

/**
 * A light runs once over the whole batch, and every lane gets what it shone. `ambientlight`
 * lights every point and is the one an ambient() call collects.
 **/
BOOST_AUTO_TEST_CASE(slglobals_shine_test) {
    ParameterList list;
    list.add("intensity", Declaration(Declaration::Storage::UNIFORM, Declaration::Type::FLOAT, 1),
        { 2.0f }, std::vector<std::string>());
    v3d::render::offline::sl::Placed light;
    light.shader = library().instance("ambientlight", ShaderType::LIGHT, list);
    BOOST_REQUIRE(light.shader);
    Machine machine;
    machine.prepare(light.shader->program(), 3);
    const Globals globals(light.shader->program());

    Value surface;
    surface.reset(v3d::render::offline::sl::Type::POINT, v3d::render::offline::sl::Storage::VARYING, 3);
    Value direction;
    direction.reset(v3d::render::offline::sl::Type::VECTOR, v3d::render::offline::sl::Storage::VARYING, 3);
    Value colour;
    colour.reset(v3d::render::offline::sl::Type::COLOR, v3d::render::offline::sl::Storage::VARYING, 3);
    std::vector<char> reached;
    bool ambient = false;
    BOOST_REQUIRE(globals.shine(light, &machine, 3, surface, &direction, &colour, &reached, &ambient));

    BOOST_CHECK(ambient);
    BOOST_REQUIRE_EQUAL(reached.size(), 3u);
    for (unsigned int lane = 0; lane < 3; lane++) {
        BOOST_CHECK(reached[lane] != 0);
        BOOST_CHECK_CLOSE(colour.triple(lane).g, 2.0f, 0.01f);
    }
}
