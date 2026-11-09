/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Description.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace {

namespace geometry = v3d::moya::geometry;
namespace scene = v3d::moya::scene;

/** The corners of a unit square, at a height. **/
scene::Value square(float z) {
    return scene::Value::points({ glm::vec3(0.0F, 0.0F, z), glm::vec3(1.0F, 0.0F, z), glm::vec3(1.0F, 1.0F, z), glm::vec3(0.0F, 1.0F, z) });
}

/** A mesh of points, the count of each face's vertices, and indices into the points. **/
scene::Handle mesh(scene::Scene & scene, const std::string & name, const scene::Value & points, const std::vector<std::int32_t> & vertices,
    const std::vector<std::int32_t> & indices) {
    const scene::Handle made = scene.create("mesh", name);
    scene.set(made, "P", points);
    scene.set(made, "vertices", scene::Value::integers(vertices));
    scene.set(made, "indices", scene::Value::integers(indices));
    return made;
}

/** An instance of a prototype in the world. **/
scene::Handle instanced(scene::Scene & scene, const std::string & name, const scene::Handle & prototype) {
    const scene::Handle made = scene.create("instance", name);
    scene.set(made, "prototype", scene::Value::handle(prototype));
    return made;
}

/** Whether a report of what gather() left out holds a phrase. **/
bool reported(const geometry::Prototypes & prototypes, const std::string & phrase) {
    return std::ranges::any_of(prototypes.reports, [&phrase](const std::string & report) { return report.find(phrase) != std::string::npos; });
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

/**
 * An instance whose prototype was removed places nothing, even when the prototype's index is used
 * again by a mesh another instance places.
 *
 * Covers the base: e8e902d8 added the walk. Checked by dropping the contains() test on the
 * prototype in Walk::instance(), which places the stale instance at the new mesh and fails this
 * case.
 **/
BOOST_AUTO_TEST_CASE(moya_scene_an_instance_of_a_removed_prototype_places_nothing_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle spare = scene.create("instance", "spare");
    const scene::Handle gone = mesh(scene, "gone", square(0.0F), { 4 }, { 0, 1, 2, 3 });
    instanced(scene, "stale", gone);
    scene.commit();
    scene.apply();

    // the new mesh takes the removed mesh's index, and its instance the spare's, which comes first
    scene.remove(spare);
    scene.remove(gone);
    const scene::Handle kept = mesh(scene, "kept", square(1.0F), { 4 }, { 0, 1, 2, 3 });
    BOOST_TEST((kept.index == gone.index));
    instanced(scene, "fresh", kept);
    scene.commit();
    scene.apply();

    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(prototypes.instances.size() == 1U);
    BOOST_TEST(prototypes.world.instances.size() == 1U);
    BOOST_TEST(prototypes.geometries.size() == 1U);
    BOOST_TEST(prototypes.reports.empty());
}

/**
 * An instance whose group was removed is in no group, so nothing places it, the world included.
 *
 * Covers the base: e8e902d8 added the walk. Checked by putting an instance of a removed group in
 * the world in Walk::byGroup(), which fails this case.
 **/
BOOST_AUTO_TEST_CASE(moya_scene_an_instance_in_a_removed_group_is_placed_by_nothing_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle leaf = mesh(scene, "leaf", square(0.0F), { 4 }, { 0, 1, 2, 3 });
    const scene::Handle branch = scene.create("group", "branch");
    const scene::Handle held = instanced(scene, "held", leaf);
    scene.set(held, "group", scene::Value::handle(branch));
    instanced(scene, "loose", leaf);
    scene.commit();
    scene.apply();

    scene.remove(branch);
    scene.commit();
    scene.apply();

    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(prototypes.instances.size() == 1U);
    BOOST_TEST(prototypes.world.instances.size() == 1U);
    BOOST_TEST(prototypes.groups.empty());
}

/**
 * A mesh's faces that cover no area are dropped and reported, and its other faces are kept.
 *
 * Covers the base: e8e902d8 added the report. Checked by removing the report from Walk::mesh(),
 * which fails this case.
 **/
BOOST_AUTO_TEST_CASE(moya_scene_a_mesh_face_that_covers_no_area_is_reported_test) {
    scene::Scene scene(scene::kinds());
    instanced(scene, "flat here", mesh(scene, "flat", square(0.0F), { 3, 3 }, { 0, 1, 2, 0, 0, 1 }));
    scene.commit();
    scene.apply();

    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(prototypes.reports.size() == 1U);
    BOOST_TEST(reported(prototypes, "mesh flat has 1 faces that cover no area"));
    BOOST_TEST(prototypes.instances.size() == 1U);
    BOOST_TEST(prototypes.faces.size() == 1U);
}

/**
 * A point cloud with a width for some of its points but not all takes its constant width, and is
 * reported.
 *
 * Covers the base: 153174d5 added point clouds. Checked by removing the report from
 * Walk::cloud(), which fails this case.
 **/
BOOST_AUTO_TEST_CASE(moya_scene_a_cloud_with_too_few_widths_takes_its_constant_width_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle dust = scene.create("points", "dust");
    scene.set(dust, "P", scene::Value::points({ glm::vec3(0.0F), glm::vec3(1.0F, 0.0F, 0.0F), glm::vec3(2.0F, 0.0F, 0.0F) }));
    scene.set(dust, "width", scene::Value::floats({ 0.5F, 0.25F }));
    scene.set(dust, "constantWidth", scene::Value::number(0.125F));
    instanced(scene, "dust here", dust);
    scene.commit();
    scene.apply();

    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(prototypes.reports.size() == 1U);
    BOOST_TEST(reported(prototypes, "points dust has a width for some of its points but not all"));
    BOOST_TEST((prototypes.widths == std::vector<float> { 0.125F, 0.125F, 0.125F }), boost::test_tools::per_element());
}

/**
 * A mesh whose faces cannot be cut from its points and indices is left out and reported, and the
 * rest of the scene is still gathered.
 **/
BOOST_AUTO_TEST_CASE(moya_scene_a_mesh_that_cannot_be_cut_is_left_out_test) {
    scene::Scene scene(scene::kinds());
    instanced(scene, "outside here", mesh(scene, "outside", square(0.0F), { 3 }, { 0, 1, 7 }));
    instanced(scene, "short here", mesh(scene, "short", square(0.0F), { 4 }, { 0, 1, 2 }));
    instanced(scene, "negative here", mesh(scene, "negative", square(0.0F), { -1 }, {}));
    instanced(scene, "good here", mesh(scene, "good", square(0.0F), { 4 }, { 0, 1, 2, 3 }));
    scene.commit();
    scene.apply();

    geometry::Prototypes prototypes;
    BOOST_CHECK_NO_THROW(prototypes = geometry::gather(scene, {}));
    BOOST_TEST(prototypes.reports.size() == 3U);
    for (const char * name : { "outside", "short", "negative" }) {
        BOOST_TEST(reported(prototypes, std::string("mesh ") + name + " is left out"), name);
    }
    BOOST_TEST(prototypes.instances.size() == 1U);
    BOOST_TEST(prototypes.geometries.size() == 1U);
    BOOST_TEST(prototypes.faces.size() == 2U);
}

};  // namespace
