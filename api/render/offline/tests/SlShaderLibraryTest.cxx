/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/offline/rib/Declarations.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/sl/ShaderLibrary.h>

#include <fstream>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

namespace {

typedef v3d::render::offline::sl::ShaderLibrary ShaderLibrary;
typedef v3d::render::offline::sl::InstancePtr InstancePtr;
typedef v3d::render::offline::sl::ShaderType ShaderType;
typedef v3d::render::offline::rib::Declaration Declaration;
typedef v3d::render::offline::rib::ParameterList ParameterList;

boost::shared_ptr<v3d::log::Logger> logger() {
    return boost::make_shared<v3d::log::Logger>();
}

/**
 * One parameter as a scene wrote it, typed the way the reader would have typed it.
 **/
void add(ParameterList* list, const std::string & name, Declaration::Type type,
    const std::vector<float> & values) {
    list->add(name, Declaration(Declaration::Storage::UNIFORM, type, 1), values,
        std::vector<std::string>());
}

/**
 * The value a bound parameter came to, read out of a machine the instance wrote into.
 **/
glm::vec3 bound(const InstancePtr & instance, const std::string & name) {
    v3d::render::offline::sl::runtime::Machine machine;
    machine.prepare(instance->program(), 1);
    instance->write(&machine);
    const int reg = instance->program().symbol(name);
    BOOST_REQUIRE_MESSAGE(reg >= 0, "no parameter named " + name);
    return machine.value(reg).triple(0);
}

};  // namespace

/**
 * Every standard shader compiles against no files at all, which is what makes a renderer's
 * suite hermetic and a first render possible with nothing installed.
 **/
BOOST_AUTO_TEST_CASE(slshaderlibrary_standard_shaders_compile_test) {
    ShaderLibrary library(logger());
    const ParameterList none;

    const char* const surfaces[] = { "constant", "matte", "metal", "plastic" };
    for (const char* const name : surfaces) {
        const InstancePtr shader = library.instance(name, ShaderType::SURFACE, none);
        BOOST_REQUIRE_MESSAGE(shader, std::string("no instance of ") + name);
        BOOST_CHECK_EQUAL(shader->name(), name);
    }

    const char* const lights[] = { "ambientlight", "distantlight", "pointlight", "spotlight" };
    for (const char* const name : lights) {
        const InstancePtr shader = library.instance(name, ShaderType::LIGHT, none);
        BOOST_REQUIRE_MESSAGE(shader, std::string("no instance of ") + name);
        BOOST_CHECK_EQUAL(shader->name(), name);
        // only the one using neither illuminate nor solar is ambient, and it is the one
        // an illuminance loop cannot see
        BOOST_CHECK_EQUAL(shader->ambient(), std::string(name) == "ambientlight");
    }

    const InstancePtr imager = library.instance("background", ShaderType::IMAGER, none);
    BOOST_REQUIRE(imager);
    BOOST_CHECK_EQUAL(imager->name(), "background");
}

/**
 * A parameter list binds onto the declared parameters and everything it does not name is
 * left at the default the shader declared - which is what `Surface "plastic" "Ks" [0.8]`
 * has to mean.
 **/
BOOST_AUTO_TEST_CASE(slshaderlibrary_binds_over_the_defaults_test) {
    ShaderLibrary library(logger());
    ParameterList list;
    add(&list, "Ks", Declaration::Type::FLOAT, { 0.8f });

    const InstancePtr shader = library.instance("plastic", ShaderType::SURFACE, list);
    BOOST_REQUIRE(shader);
    BOOST_CHECK_CLOSE(bound(shader, "Ks").x, 0.8f, 0.01f);
    // the four the scene did not name keep what plastic declares
    BOOST_CHECK_CLOSE(bound(shader, "Ka").x, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(bound(shader, "Kd").x, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(bound(shader, "roughness").x, 0.1f, 0.01f);
    // "color specularcolor = 1" is a float promoted into a colour by replication
    BOOST_CHECK_CLOSE(bound(shader, "specularcolor").b, 1.0f, 0.01f);
}

/**
 * A default written as an expression is a value, because the program computes it in a
 * prologue the run itself skips. `point "shader" (0, 0, 1)` is the case that matters:
 * distantlight aims itself with two of them.
 **/
BOOST_AUTO_TEST_CASE(slshaderlibrary_expression_default_test) {
    ShaderLibrary library(logger());
    const ParameterList none;

    const InstancePtr shader = library.instance("distantlight", ShaderType::LIGHT, none);
    BOOST_REQUIRE(shader);
    BOOST_CHECK_SMALL(bound(shader, "from").z, 0.0001f);
    BOOST_CHECK_CLOSE(bound(shader, "to").z, 1.0f, 0.01f);
}

/**
 * A colour bound with one value replicates and one bound with three does not, which is
 * RI's promotion rather than a partial write.
 **/
BOOST_AUTO_TEST_CASE(slshaderlibrary_binds_a_colour_test) {
    ShaderLibrary library(logger());
    ParameterList list;
    add(&list, "lightcolor", Declaration::Type::COLOR, { 0.25f, 0.5f, 0.75f });

    const InstancePtr shader = library.instance("ambientlight", ShaderType::LIGHT, list);
    BOOST_REQUIRE(shader);
    BOOST_CHECK_CLOSE(bound(shader, "lightcolor").r, 0.25f, 0.01f);
    BOOST_CHECK_CLOSE(bound(shader, "lightcolor").g, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(bound(shader, "lightcolor").b, 0.75f, 0.01f);
}

/**
 * A name the shader does not declare is dropped rather than refused: a renderer must accept
 * a request carrying a parameter it does not support. A type that will not coerce is
 * dropped too, and the parameter keeps its default rather than being reinterpreted.
 **/
BOOST_AUTO_TEST_CASE(slshaderlibrary_unbindable_parameters_test) {
    ShaderLibrary library(logger());
    ParameterList list;
    add(&list, "Kwhatever", Declaration::Type::FLOAT, { 3.0f });
    add(&list, "Kd", Declaration::Type::MATRIX, std::vector<float>(16, 2.0f));

    const InstancePtr shader = library.instance("matte", ShaderType::SURFACE, list);
    BOOST_REQUIRE(shader);
    // a matrix has no reading as a float, so Kd keeps what matte declares
    BOOST_CHECK_CLOSE(bound(shader, "Kd").x, 1.0f, 0.01f);
}

/**
 * A shader that will not compile is reported and substituted rather than fatal - RI asks a
 * renderer to carry on. A light has no substitute, because a light of some other kind is a
 * worse answer than one fewer light.
 **/
BOOST_AUTO_TEST_CASE(slshaderlibrary_substitutes_a_failure_test) {
    ShaderLibrary library(logger());
    const ParameterList none;

    const InstancePtr missing = library.instance("nosuchshader", ShaderType::SURFACE, none);
    BOOST_REQUIRE(missing);
    BOOST_CHECK_EQUAL(missing->name(), "matte");

    BOOST_CHECK(!library.instance("nosuchlight", ShaderType::LIGHT, none));

    // a name that is a shader of another kind is the same answer
    const InstancePtr wrong = library.instance("distantlight", ShaderType::SURFACE, none);
    BOOST_REQUIRE(wrong);
    BOOST_CHECK_EQUAL(wrong->name(), "matte");
}

/**
 * A `.sl` file on the search path wins over a built-in of the same name, which is how a
 * scene replaces one. `&` in the path is whatever it was before, which is how a scene adds
 * to what a driver put there rather than replacing it.
 **/
BOOST_AUTO_TEST_CASE(slshaderlibrary_searchpath_test) {
    {
        std::ofstream file("matte.sl");
        BOOST_REQUIRE(file.is_open());
        file << "surface matte(float Kd = 7) { Ci = Cs * Kd; }\n";
    }

    ShaderLibrary library(logger());
    library.searchpath(".");
    const ParameterList none;
    const InstancePtr shader = library.instance("matte", ShaderType::SURFACE, none);
    BOOST_REQUIRE(shader);
    // the file's default rather than the built-in's, which says which one was compiled
    BOOST_CHECK_CLOSE(bound(shader, "Kd").x, 7.0f, 0.01f);

    ShaderLibrary appended(logger());
    appended.searchpath("nowhere");
    appended.searchpath(".:&");
    const InstancePtr again = appended.instance("matte", ShaderType::SURFACE, none);
    BOOST_REQUIRE(again);
    BOOST_CHECK_CLOSE(bound(again, "Kd").x, 7.0f, 0.01f);

    std::remove("matte.sl");
}
