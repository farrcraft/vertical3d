/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace {

namespace scene = v3d::moya::scene;

/**
 * Two kinds the tests make objects of: a shape that holds primvars, a transformation over the
 * shutter and a handle of a look, and the look, which holds a colour.
 **/
scene::Schema schema() {
    scene::Schema made;
    made.add(scene::Kind { .name = "look",
        .parameters = { scene::Declaration { .name = "colour", .type = scene::Type::Colour, .fallback = scene::Value::colour(glm::vec3(0.5F)) } } });
    made.add(scene::Kind { .name = "shape",
        .parameters = {
            scene::Declaration { .name = "transform", .type = scene::Type::Matrix, .motion = true, .fallback = scene::Value::matrix(glm::mat4(1.0F)) },
            scene::Declaration { .name = "P", .type = scene::Type::Point, .count = scene::Declaration::kAny, .motion = true, .fallback = scene::Value::points({}) },
            scene::Declaration { .name = "sides", .type = scene::Type::Integer, .fallback = scene::Value::integer(1) },
            scene::Declaration { .name = "look", .type = scene::Type::Handle, .fallback = scene::Value::handle({}), .target = "look" } },
        .primvars = true });
    return made;
}

/** The scene's changes after a version, as the names of what happened, for comparing. **/
std::vector<std::string> what(const scene::Scene & scene, std::uint64_t since) {
    std::vector<std::string> found;
    for (const scene::Change & change : scene.changes(since)) {
        std::string line = std::to_string(change.handle.index) + " ";
        switch (change.what) {
            case scene::Change::What::Created: line += "created"; break;
            case scene::Change::What::Modified: line += "modified"; break;
            case scene::Change::What::Removed: line += "removed"; break;
        }
        for (const std::string & parameter : change.parameters) {
            line += " " + parameter;
        }
        found.push_back(line);
    }
    return found;
}

BOOST_AUTO_TEST_CASE(moya_scene_an_object_holds_its_fallbacks_once_applied_test) {
    scene::Scene scene(schema());
    const scene::Handle shape = scene.create("shape", "shape 1");
    BOOST_TEST(shape.valid());
    BOOST_TEST(!scene.contains(shape));

    scene.commit();
    BOOST_TEST(scene.apply() == 1U);
    BOOST_TEST(scene.contains(shape));
    BOOST_TEST((scene.value(shape, "sides") == scene::Value::integer(1)));
    BOOST_TEST(scene.samples(shape, "transform").times.empty());
    BOOST_TEST(scene.objects("shape").size() == 1U);
    BOOST_TEST(scene.objects("look").empty());
}

BOOST_AUTO_TEST_CASE(moya_scene_an_edit_waits_for_its_commit_test) {
    scene::Scene scene(schema());
    const scene::Handle shape = scene.create("shape", "shape 2");
    scene.commit();
    scene.apply();

    scene.set(shape, "sides", scene::Value::integer(4));
    BOOST_TEST(scene.apply() == 1U);
    BOOST_TEST((scene.value(shape, "sides") == scene::Value::integer(1)));

    scene.commit();
    BOOST_TEST(scene.apply() == 2U);
    BOOST_TEST((scene.value(shape, "sides") == scene::Value::integer(4)));
}

BOOST_AUTO_TEST_CASE(moya_scene_a_removed_handle_is_refused_test) {
    scene::Scene scene(schema());
    const scene::Handle shape = scene.create("shape", "shape 3");
    scene.remove(shape);
    BOOST_CHECK_THROW(scene.set(shape, "sides", scene::Value::integer(2)), std::invalid_argument);
    BOOST_CHECK_THROW(scene.remove(shape), std::invalid_argument);

    // the index is used again with the next generation, which the old handle does not match
    const scene::Handle next = scene.create("shape", "shape 4");
    BOOST_TEST((next.index == shape.index));
    BOOST_TEST((next.generation == shape.generation + 1U));
    BOOST_CHECK_THROW(scene.set(shape, "sides", scene::Value::integer(2)), std::invalid_argument);
    scene.set(next, "sides", scene::Value::integer(2));

    scene.commit();
    scene.apply();
    BOOST_TEST(!scene.contains(shape));
    BOOST_TEST(scene.contains(next));
    BOOST_CHECK_THROW(scene.samples(shape, "sides"), std::out_of_range);

    const scene::Handle unmade;
    BOOST_CHECK_THROW(scene.set(unmade, "sides", scene::Value::integer(2)), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(moya_scene_a_value_that_breaks_its_declaration_is_refused_test) {
    scene::Scene scene(schema());
    const scene::Handle shape = scene.create("shape", "shape 5");
    const scene::Handle look = scene.create("look", "look 6");
    BOOST_CHECK_THROW(scene.create("nothing", "nothing 7"), std::invalid_argument);
    BOOST_CHECK_THROW(scene.set(shape, "missing", scene::Value::integer(1)), std::invalid_argument);
    BOOST_CHECK_THROW(scene.set(shape, "sides", scene::Value::number(1.0F)), std::invalid_argument);
    BOOST_CHECK_THROW(scene.set(shape, "sides", scene::Value::integers({ 1, 2 })), std::invalid_argument);
    BOOST_CHECK_THROW(scene.set(shape, "sides", scene::Value::integers({})), std::invalid_argument);
    BOOST_CHECK_THROW(scene.set(shape, "look", scene::Value::handle(shape)), std::invalid_argument);
    BOOST_CHECK_THROW(scene.set(shape, "look", scene::Value::handle({})), std::invalid_argument);
    scene.set(shape, "look", scene::Value::handle(look));
    scene.set(shape, "P", scene::Value::points({ glm::vec3(0.0F), glm::vec3(1.0F) }));
}

BOOST_AUTO_TEST_CASE(moya_scene_samples_over_the_shutter_keep_their_times_test) {
    scene::Scene scene(schema());
    const scene::Handle shape = scene.create("shape", "shape 8");
    const std::vector<float> times = { 0.0F, 0.25F, 1.0F };
    const std::vector<scene::Value> points = {
        scene::Value::points({ glm::vec3(0.0F), glm::vec3(1.0F) }),
        scene::Value::points({ glm::vec3(0.5F), glm::vec3(1.5F) }),
        scene::Value::points({ glm::vec3(2.0F), glm::vec3(3.0F) }) };
    scene.set(shape, "P", times, points);
    scene.commit();
    scene.apply();
    BOOST_TEST(scene.samples(shape, "P").times == times);
    BOOST_TEST(scene.samples(shape, "P").values.size() == 3U);
    BOOST_TEST((scene.samples(shape, "P").values[1].triple(1) == glm::vec3(1.5F)));

    // the times rise strictly, there is a value at each, and each sample has the same size
    BOOST_CHECK_THROW(scene.set(shape, "P", { 0.0F, 0.0F }, { points[0], points[1] }), std::invalid_argument);
    BOOST_CHECK_THROW(scene.set(shape, "P", { 0.0F, 1.0F }, { points[0] }), std::invalid_argument);
    BOOST_CHECK_THROW(scene.set(shape, "P", { 0.0F, 1.0F }, { points[0], scene::Value::points({ glm::vec3(0.0F) }) }), std::invalid_argument);
    BOOST_CHECK_THROW(scene.set(shape, "P", {}, {}), std::invalid_argument);

    // a parameter not declared for motion takes one value at every time
    BOOST_CHECK_THROW(scene.set(shape, "sides", { 0.0F, 1.0F }, { scene::Value::integer(1), scene::Value::integer(2) }), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(moya_scene_setting_a_parameter_changes_it_alone_test) {
    scene::Scene scene(schema());
    const scene::Handle shape = scene.create("shape", "shape 9");
    const scene::Handle other = scene.create("shape", "shape 10");
    scene.commit();
    const std::uint64_t built = scene.apply();
    BOOST_TEST(what(scene, 0) == (std::vector<std::string> { "0 created", "1 created" }), boost::test_tools::per_element());

    scene.set(shape, "sides", scene::Value::integer(3));
    scene.commit();
    scene.apply();
    BOOST_TEST(what(scene, built) == (std::vector<std::string> { "0 modified sides" }), boost::test_tools::per_element());
    BOOST_TEST((scene.value(other, "sides") == scene::Value::integer(1)));
}

BOOST_AUTO_TEST_CASE(moya_scene_two_consumers_each_see_every_change_since_their_own_version_test) {
    scene::Scene scene(schema());
    const scene::Handle shape = scene.create("shape", "shape 11");
    scene.commit();
    scene.apply();

    // one consumer builds now, and the other has not built at all
    const std::uint64_t first = scene.version();
    scene.set(shape, "sides", scene::Value::integer(3));
    scene.commit();
    scene.apply();
    const std::uint64_t second = scene.version();
    scene.set(shape, "transform", scene::Value::matrix(glm::mat4(2.0F)));
    scene.commit();
    scene.apply();

    BOOST_TEST(what(scene, first) == (std::vector<std::string> { "0 modified sides transform" }), boost::test_tools::per_element());
    BOOST_TEST(what(scene, second) == (std::vector<std::string> { "0 modified transform" }), boost::test_tools::per_element());
    BOOST_TEST(what(scene, 0) == (std::vector<std::string> { "0 created" }), boost::test_tools::per_element());
    BOOST_TEST(scene.changes(scene.version()).empty());
}

BOOST_AUTO_TEST_CASE(moya_scene_a_commit_of_many_edits_is_one_version_test) {
    scene::Scene scene(schema());
    constexpr int kShapes = 8;
    std::vector<scene::Handle> shapes;
    shapes.reserve(kShapes);
    for (int i = 0; i < kShapes; ++i) {
        shapes.push_back(scene.create("shape", "shape " + std::to_string(i)));
    }
    for (const scene::Handle & shape : shapes) {
        scene.set(shape, "sides", scene::Value::integer(5));
    }
    scene.commit();
    scene.commit();
    BOOST_TEST(scene.apply() == 1U);

    // two batches committed before an apply are two versions
    scene.set(shapes[0], "sides", scene::Value::integer(6));
    scene.commit();
    scene.set(shapes[1], "sides", scene::Value::integer(6));
    scene.commit();
    BOOST_TEST(scene.apply() == 3U);
    BOOST_TEST(what(scene, 2) == (std::vector<std::string> { "1 modified sides" }), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(moya_scene_a_removal_is_reported_before_what_takes_its_index_test) {
    scene::Scene scene(schema());
    const scene::Handle shape = scene.create("shape", "shape 13");
    scene.commit();
    const std::uint64_t built = scene.apply();

    scene.remove(shape);
    const scene::Handle next = scene.create("look", "look 14");
    scene.commit();
    scene.apply();
    BOOST_TEST((next.index == shape.index));
    BOOST_TEST(what(scene, built) == (std::vector<std::string> { "0 removed", "0 created" }), boost::test_tools::per_element());
    BOOST_TEST((scene.changes(built).front().handle == shape));

    scene.forget(scene.version());
    BOOST_TEST(what(scene, built) == (std::vector<std::string> { "0 created" }), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(moya_scene_a_primvar_is_declared_per_object_test) {
    scene::Scene scene(schema());
    const scene::Handle shape = scene.create("shape", "shape 15");
    const scene::Handle other = scene.create("shape", "shape 16");
    const scene::Handle look = scene.create("look", "look 17");
    const scene::Declaration st { .name = "st",
        .type = scene::Type::Float,
        .count = scene::Declaration::kAny,
        .fallback = scene::Value::floats({}),
        .interpolation = scene::Interpolation::FaceVarying };
    scene.declare(shape, st);
    scene.set(shape, "st", scene::Value::floats({ 0.0F, 1.0F }));
    BOOST_CHECK_THROW(scene.set(other, "st", scene::Value::floats({ 0.0F })), std::invalid_argument);
    BOOST_CHECK_THROW(scene.declare(look, st), std::invalid_argument);
    BOOST_CHECK_THROW(scene.declare(shape, scene::Declaration { .name = "sides", .type = scene::Type::Integer, .fallback = scene::Value::integer(0) }),
        std::invalid_argument);
    BOOST_CHECK_THROW(scene.declare(shape, scene::Declaration { .name = "Cs", .type = scene::Type::Colour, .fallback = scene::Value::number(0.0F) }),
        std::invalid_argument);

    scene.commit();
    scene.apply();
    BOOST_TEST(scene.primvars(shape).size() == 1U);
    BOOST_TEST((scene.primvars(shape).front().interpolation == scene::Interpolation::FaceVarying));
    BOOST_TEST((scene.value(shape, "st") == scene::Value::floats({ 0.0F, 1.0F })));
    BOOST_TEST(scene.primvars(other).empty());
}

BOOST_AUTO_TEST_CASE(moya_scene_a_schema_refuses_a_broken_kind_test) {
    scene::Schema made = schema();
    BOOST_CHECK_THROW(made.add(scene::Kind { .name = "look" }), std::invalid_argument);
    BOOST_CHECK_THROW(made.add(scene::Kind { .name = "" }), std::invalid_argument);
    const scene::Declaration size { .name = "size", .type = scene::Type::Float, .fallback = scene::Value::number(1.0F) };
    BOOST_CHECK_THROW(made.add(scene::Kind { .name = "twice", .parameters = { size, size } }), std::invalid_argument);
    BOOST_CHECK_THROW(made.add(scene::Kind { .name = "empty",
                          .parameters = { scene::Declaration { .name = "size", .type = scene::Type::Float, .fallback = scene::Value::floats({}) } } }),
        std::invalid_argument);
    BOOST_CHECK_THROW(made.add(scene::Kind { .name = "pointer",
                          .parameters = { scene::Declaration {
                              .name = "to", .type = scene::Type::Handle, .fallback = scene::Value::handle({}), .target = "nothing" } } }),
        std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(moya_scene_clients_on_several_threads_edit_one_scene_test) {
    scene::Scene scene(schema());
    constexpr int kThreads = 4;
    constexpr int kEach = 64;
    std::vector<std::thread> clients;
    clients.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        clients.emplace_back([&scene, t]() {
            for (int i = 0; i < kEach; ++i) {
                const scene::Handle shape = scene.create("shape", "shape " + std::to_string(t) + " " + std::to_string(i));
                scene.set(shape, "sides", scene::Value::integer(i));
                scene.commit();
            }
        });
    }
    // the owner applies while the clients edit
    for (int i = 0; i < kEach; ++i) {
        scene.apply();
    }
    for (std::thread & client : clients) {
        client.join();
    }
    scene.apply();
    BOOST_TEST(scene.objects("shape").size() == static_cast<std::size_t>(kThreads * kEach));
}

BOOST_AUTO_TEST_CASE(moya_scene_a_name_is_unique_within_its_kind_test) {
    scene::Scene scene(schema());
    const scene::Handle shape = scene.create("shape", "one");
    const scene::Handle look = scene.create("look", "one");
    BOOST_CHECK_THROW(scene.create("shape", "one"), std::invalid_argument);
    BOOST_CHECK_THROW(scene.create("shape", ""), std::invalid_argument);
    scene.commit();
    scene.apply();
    BOOST_TEST(scene.name(shape) == "one");
    BOOST_TEST((scene.find("shape", "one") == shape));
    BOOST_TEST((scene.find("look", "one") == look));
    BOOST_TEST(!scene.find("shape", "two").has_value());
    BOOST_TEST(!scene.find("nothing", "one").has_value());

    // a removed object's name is free again
    scene.remove(shape);
    const scene::Handle again = scene.create("shape", "one");
    scene.commit();
    scene.apply();
    BOOST_TEST((scene.find("shape", "one") == again));
    BOOST_CHECK_THROW(scene.name(shape), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(moya_scene_a_value_reads_back_as_it_was_made_test) {
    glm::mat4 matrix(1.0F);
    matrix[3] = glm::vec4(1.0F, 2.0F, 3.0F, 1.0F);
    const scene::Value matrices = scene::Value::matrix(matrix);
    BOOST_TEST(matrices.size() == 1U);
    BOOST_TEST(matrices.floatArray().size() == 16U);
    BOOST_TEST((matrices.matrixAt(0) == matrix));
    BOOST_CHECK_THROW(matrices.matrixAt(1), std::out_of_range);

    const scene::Value colour = scene::Value::colour(glm::vec3(0.1F, 0.2F, 0.3F), "srgb_tx");
    BOOST_TEST(colour.space() == "srgb_tx");
    BOOST_TEST((colour.triple(0) == glm::vec3(0.1F, 0.2F, 0.3F)));
    BOOST_CHECK_THROW(colour.integerArray(), std::logic_error);
    BOOST_TEST((!(colour == scene::Value::colour(glm::vec3(0.1F, 0.2F, 0.3F)))));

    BOOST_TEST((scene::Value::strings({ "a", "b" }).size() == 2U));
    BOOST_TEST((scene::Value().type() == scene::Type::Integer));
    BOOST_TEST(scene::Value().size() == 0U);
}

};  // namespace
