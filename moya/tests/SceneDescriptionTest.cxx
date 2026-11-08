/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/scene/Description.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace {

namespace scene = v3d::moya::scene;

/** A kind of every type of parameter, and a kind that holds primvars and names the first. **/
scene::Schema schema() {
    scene::Schema made;
    made.add(scene::Kind { .name = "look",
        .parameters = {
            scene::Declaration { .name = "colour", .type = scene::Type::Colour, .fallback = scene::Value::colour(glm::vec3(0.5F, 0.25F, 1.0F), "srgb_tx") },
            scene::Declaration { .name = "count", .type = scene::Type::Integer, .fallback = scene::Value::integer(-3) },
            scene::Declaration { .name = "label", .type = scene::Type::String, .fallback = scene::Value::string("say \"hi\"") },
            scene::Declaration { .name = "normal", .type = scene::Type::Normal, .fallback = scene::Value::normal(glm::vec3(0.0F, 0.0F, 1.0F)) },
            scene::Declaration { .name = "roughness", .type = scene::Type::Float, .fallback = scene::Value::number(0.1F) },
            scene::Declaration { .name = "up", .type = scene::Type::Vector, .fallback = scene::Value::vector(glm::vec3(0.0F, 1.0F, 0.0F)) } } });
    made.add(scene::Kind { .name = "shape",
        .parameters = {
            scene::Declaration { .name = "P", .type = scene::Type::Point, .count = scene::Declaration::kAny, .motion = true, .fallback = scene::Value::points({}) },
            scene::Declaration { .name = "transform", .type = scene::Type::Matrix, .motion = true, .fallback = scene::Value::matrix(glm::mat4(1.0F)) },
            scene::Declaration { .name = "look", .type = scene::Type::Handle, .fallback = scene::Value::handle({}), .targets = { "look" } } },
        .primvars = true });
    return made;
}

/** A shape, a second shape and a look, made in the order given. **/
std::string built(bool lookFirst) {
    scene::Scene scene(schema());
    scene::Handle look;
    if (lookFirst) {
        look = scene.create("look", "red");
    }
    const scene::Handle second = scene.create("shape", "b");
    const scene::Handle first = scene.create("shape", "a");
    if (!lookFirst) {
        look = scene.create("look", "red");
    }
    scene.set(first, "look", scene::Value::handle(look));
    scene.set(first, "P", { 0.0F, 0.5F }, { scene::Value::points({ glm::vec3(0.0F), glm::vec3(1.0F, 2.0F, 3.0F) }),
                                              scene::Value::points({ glm::vec3(0.5F), glm::vec3(1.5F, 2.5F, 3.5F) }) });
    scene.declare(second, scene::Declaration { .name = "st",
        .type = scene::Type::Float,
        .count = scene::Declaration::kAny,
        .fallback = scene::Value::floats({}),
        .interpolation = scene::Interpolation::FaceVarying });
    scene.set(second, "st", scene::Value::floats({ 0.25F, 1.0F }));
    scene.commit();
    scene.apply();
    return describe(scene);
}

BOOST_AUTO_TEST_CASE(moya_scene_two_scenes_built_in_different_orders_describe_alike_test) {
    BOOST_TEST(built(true) == built(false));
}

BOOST_AUTO_TEST_CASE(moya_scene_a_description_names_every_parameter_and_value_test) {
    const std::string expected =
        "look \"red\"\n"
        "    colour colour = (0.5, 0.25, 1) in \"srgb_tx\"\n"
        "    count integer = -3\n"
        "    label string = \"say \\\"hi\\\"\"\n"
        "    normal normal = (0, 0, 1)\n"
        "    roughness float = 0.1\n"
        "    up vector = (0, 1, 0)\n"
        "shape \"a\"\n"
        "    P point[2] @ 0 = (0, 0, 0) (1, 2, 3) @ 0.5 = (0.5, 0.5, 0.5) (1.5, 2.5, 3.5)\n"
        "    look handle = look \"red\"\n"
        "    transform matrix = [1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]\n"
        "shape \"b\"\n"
        "    P point[0] =\n"
        "    look handle = none\n"
        "    st float[2] facevarying = 0.25 1\n"
        "    transform matrix = [1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]\n";
    BOOST_TEST(built(true) == expected);
}

BOOST_AUTO_TEST_CASE(moya_scene_a_description_holds_only_applied_objects_test) {
    scene::Scene scene(schema());
    scene.create("look", "red");
    BOOST_TEST(describe(scene).empty());
    scene.commit();
    scene.apply();
    BOOST_TEST(describe(scene).starts_with("look \"red\"\n"));

    // a handle to an object removed after it was set names nothing
    const scene::Handle look = *scene.find("look", "red");
    const scene::Handle shape = scene.create("shape", "a");
    scene.set(shape, "look", scene::Value::handle(look));
    scene.remove(look);
    scene.commit();
    scene.apply();
    BOOST_TEST(describe(scene).find("look handle = none") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(moya_scene_a_description_writes_six_digits_and_no_negative_zero_test) {
    scene::Scene scene(schema());
    const scene::Handle look = scene.create("look", "grey");
    scene.set(look, "roughness", scene::Value::number(1.0F / 3.0F));
    scene.set(look, "normal", scene::Value::normal(glm::vec3(-0.0F, 123456789.0F, 1.0e-7F)));
    scene.commit();
    scene.apply();
    const std::string text = describe(scene);
    BOOST_TEST(text.find("    roughness float = 0.333333\n") != std::string::npos);
    BOOST_TEST(text.find("    normal normal = (0, 1.23457e+08, 1e-07)\n") != std::string::npos);
}

};  // namespace
