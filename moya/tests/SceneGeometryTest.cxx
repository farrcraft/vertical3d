/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/scene/Description.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace {

namespace scene = v3d::moya::scene;

/** The corners of a unit square, at a height. **/
scene::Value square(float z) {
    return scene::Value::points({ glm::vec3(0.0F, 0.0F, z), glm::vec3(1.0F, 0.0F, z), glm::vec3(1.0F, 1.0F, z), glm::vec3(0.0F, 1.0F, z) });
}

/** The lines of a description that hold a word, in order. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the text, then what to find in it
std::vector<std::string> lines(const std::string & text, const std::string & word) {
    std::vector<std::string> found;
    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t end = text.find('\n', start);
        if (end == std::string::npos) {
            end = text.size();
        }
        const std::string line = text.substr(start, end - start);
        if (line.find(word) != std::string::npos) {
            found.push_back(line);
        }
        start = end + 1;
    }
    return found;
}

BOOST_AUTO_TEST_CASE(moya_scene_a_mesh_holds_its_points_topology_and_primvars_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle mesh = scene.create("mesh", "square");
    scene.set(mesh, "P", square(0.0F));
    scene.set(mesh, "vertices", scene::Value::integers({ 4 }));
    scene.set(mesh, "indices", scene::Value::integers({ 0, 1, 2, 3 }));
    scene.declare(mesh, scene::Declaration { .name = "Cs",
        .type = scene::Type::Colour,
        .count = scene::Declaration::kAny,
        .fallback = scene::Value::colours({}),
        .interpolation = scene::Interpolation::Uniform });
    scene.set(mesh, "Cs", scene::Value::colours({ glm::vec3(1.0F, 0.0F, 0.0F) }));
    scene.commit();
    scene.apply();

    const std::string expected =
        "mesh \"square\"\n"
        "    Cs colour[1] uniform = (1, 0, 0)\n"
        "    P point[4] = (0, 0, 0) (1, 0, 0) (1, 1, 0) (0, 1, 0)\n"
        "    indices integer[4] = 0 1 2 3\n"
        "    vertices integer[1] = 4\n";
    BOOST_TEST(describe(scene) == expected);
}

BOOST_AUTO_TEST_CASE(moya_scene_a_deforming_mesh_keeps_each_time_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle mesh = scene.create("mesh", "square");
    const std::vector<float> times = { 0.0F, 0.5F, 1.0F };
    scene.set(mesh, "P", times, { square(0.0F), square(1.0F), square(2.0F) });
    scene.commit();
    scene.apply();
    BOOST_TEST(scene.samples(mesh, "P").times == times);
    BOOST_TEST((scene.samples(mesh, "P").values[2].triple(3) == glm::vec3(0.0F, 1.0F, 2.0F)));

    // a deformation keeps its topology, so every time has the same number of points
    const scene::Value triangle = scene::Value::points({ glm::vec3(0.0F), glm::vec3(1.0F, 0.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F) });
    BOOST_CHECK_THROW(scene.set(mesh, "P", { 0.0F, 1.0F }, { square(0.0F), triangle }), std::invalid_argument);

    // the topology itself holds at every time
    BOOST_CHECK_THROW(scene.set(mesh, "vertices", { 0.0F, 1.0F }, { scene::Value::integers({ 4 }), scene::Value::integers({ 4 }) }),
        std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(moya_scene_every_prototype_kind_holds_what_it_is_given_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle subdivision = scene.create("subdivision", "cage");
    scene.set(subdivision, "scheme", scene::Value::string("loop"));
    scene.set(subdivision, "creases", scene::Value::integers({ 0, 1, 2 }));
    scene.set(subdivision, "creaseLengths", scene::Value::integers({ 3 }));
    scene.set(subdivision, "creaseSharpness", scene::Value::floats({ 2.5F }));
    BOOST_CHECK_THROW(scene.set(subdivision, "boundary", scene::Value::string("smooth")), std::invalid_argument);

    const scene::Handle curves = scene.create("curves", "hair");
    scene.set(curves, "type", scene::Value::string("cubic"));
    scene.set(curves, "basis", scene::Value::string("catmull-rom"));
    scene.set(curves, "profile", scene::Value::string("round"));
    scene.set(curves, "width", scene::Value::floats({ 0.1F, 0.05F }));
    BOOST_CHECK_THROW(scene.set(curves, "basis", scene::Value::string("nurbs")), std::invalid_argument);

    const scene::Handle points = scene.create("points", "dust");
    scene.set(points, "constantWidth", scene::Value::number(0.01F));

    const scene::Handle sphere = scene.create("sphere", "ball");
    scene.set(sphere, "radius", { 0.0F, 1.0F }, { scene::Value::number(1.0F), scene::Value::number(2.0F) });
    const scene::Handle torus = scene.create("torus", "ring");
    scene.set(torus, "phimax", scene::Value::number(180.0F));
    for (const char * quadric : { "cone", "cylinder", "hyperboloid", "paraboloid", "disk" }) {
        scene.create(quadric, std::string("a ") + quadric);
    }
    scene.commit();
    scene.apply();

    BOOST_TEST((scene.value(subdivision, "scheme") == scene::Value::string("loop")));
    BOOST_TEST((scene.value(subdivision, "boundary") == scene::Value::string("none")));
    BOOST_TEST((scene.value(curves, "basis") == scene::Value::string("catmull-rom")));
    BOOST_TEST((scene.value(points, "constantWidth") == scene::Value::number(0.01F)));
    BOOST_TEST(scene.samples(sphere, "radius").values.size() == 2U);
    BOOST_TEST((scene.value(torus, "minorRadius") == scene::Value::number(0.25F)));
    BOOST_TEST(scene.objects("hyperboloid").size() == 1U);
    BOOST_TEST((lines(describe(scene), "radius float @") == std::vector<std::string> { "    radius float @ 0 = 1 @ 1 = 2" }),
        boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(moya_scene_an_instance_places_a_prototype_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle mesh = scene.create("mesh", "square");
    const scene::Handle camera = scene.create("camera", "main");
    const scene::Handle placed = scene.create("instance", "square here");
    scene.set(placed, "prototype", scene::Value::handle(mesh));
    scene.set(placed, "transform", { 0.0F, 1.0F },
        { scene::Value::matrix(glm::mat4(1.0F)), scene::Value::matrix(glm::translate(glm::mat4(1.0F), glm::vec3(2.0F, 0.0F, 0.0F))) });
    scene.set(placed, "shadow", scene::Value::integer(0));
    BOOST_CHECK_THROW(scene.set(placed, "prototype", scene::Value::handle(camera)), std::invalid_argument);
    BOOST_CHECK_THROW(scene.set(placed, "group", scene::Value::handle(mesh)), std::invalid_argument);
    scene.commit();
    scene.apply();

    const std::vector<std::string> expected = {
        "    prototype handle = mesh \"square\"",
        "    transform matrix @ 0 = [1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1] @ 1 = [1 0 0 0 0 1 0 0 0 0 1 0 2 0 0 1]" };
    const std::string text = describe(scene);
    BOOST_TEST((lines(text, "prototype handle") == std::vector<std::string> { expected[0] }), boost::test_tools::per_element());
    BOOST_TEST((lines(text, "transform matrix @") == std::vector<std::string> { expected[1] }), boost::test_tools::per_element());
    BOOST_TEST((lines(text, "shadow") == std::vector<std::string> { "    shadow integer = 0" }), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(moya_scene_instances_nest_through_groups_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle mesh = scene.create("mesh", "leaf");
    const scene::Handle branch = scene.create("group", "branch");

    // two leaves in the branch, and the branch placed twice in the world
    for (const char * leaf : { "leaf 1", "leaf 2" }) {
        const scene::Handle placed = scene.create("instance", leaf);
        scene.set(placed, "prototype", scene::Value::handle(mesh));
        scene.set(placed, "group", scene::Value::handle(branch));
    }
    for (const char * copy : { "branch 1", "branch 2" }) {
        const scene::Handle placed = scene.create("instance", copy);
        scene.set(placed, "prototype", scene::Value::handle(branch));
    }
    scene.commit();
    scene.apply();

    const std::string text = describe(scene);
    BOOST_TEST((lines(text, "group handle") == std::vector<std::string> {
                   "    group handle = none", "    group handle = none", "    group handle = group \"branch\"", "    group handle = group \"branch\"" }),
        boost::test_tools::per_element());
    BOOST_TEST((lines(text, "prototype handle") == std::vector<std::string> { "    prototype handle = group \"branch\"",
                   "    prototype handle = group \"branch\"", "    prototype handle = mesh \"leaf\"", "    prototype handle = mesh \"leaf\"" }),
        boost::test_tools::per_element());
    BOOST_TEST(scene.objects("instance").size() == 4U);
}

BOOST_AUTO_TEST_CASE(moya_scene_a_handle_to_a_removed_prototype_names_nothing_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle mesh = scene.create("mesh", "square");
    const scene::Handle placed = scene.create("instance", "square here");
    scene.set(placed, "prototype", scene::Value::handle(mesh));
    scene.commit();
    scene.apply();

    scene.remove(mesh);
    scene.commit();
    scene.apply();
    BOOST_TEST(!scene.contains(scene.value(placed, "prototype").handleArray().front()));
    BOOST_TEST((lines(describe(scene), "prototype handle") == std::vector<std::string> { "    prototype handle = none" }),
        boost::test_tools::per_element());
}

};  // namespace
