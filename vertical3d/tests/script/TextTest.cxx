/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/Index.h>
#include <api/brep/Primitives.h>
#include <api/curve/Curve.h>
#include <api/curve/Curves.h>
#include <vertical3d/src/scene/item/Components.h>
#include <vertical3d/src/scene/item/Curve.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/script/Arguments.h>
#include <vertical3d/src/script/Text.h>

#include <optional>
#include <string>
#include <vector>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

using v3d::editor::scene::item::Component;
using v3d::editor::script::Part;
using v3d::editor::script::Type;

/**
 * A scene of a cube called Cube and another called Box.
 **/
struct Cubes {
    Cubes() :
        cube(boost::make_shared<v3d::editor::scene::item::Mesh>(v3d::brep::cube())),
        box(boost::make_shared<v3d::editor::scene::item::Mesh>(v3d::brep::cube())) {
        cube->name("Cube");
        box->name("Box");
        scene.add(cube);
        scene.add(box);
    }

    v3d::editor::scene::Scene scene;
    boost::shared_ptr<v3d::editor::scene::item::Mesh> cube;
    boost::shared_ptr<v3d::editor::scene::item::Mesh> box;
};

BOOST_AUTO_TEST_SUITE(script_text_test)

/**
 * A command is called by its name with each "::" a dot, and its values are written in its
 * parameters' order, as a script would write them.
 **/
BOOST_AUTO_TEST_CASE(a_run_is_written_as_the_line_that_runs_it) {
    BOOST_CHECK_EQUAL(v3d::editor::script::path("create::poly::cube"), "create.poly.cube");
    BOOST_CHECK_EQUAL(v3d::editor::script::line("create::poly::cube", {}, {}), "create.poly.cube()");

    const std::vector<v3d::editor::script::Parameter> keyed{ { "items", Type::Items }, { "frame", Type::Integer } };
    BOOST_CHECK_EQUAL(v3d::editor::script::line("timeline::set", keyed, boost::json::parse(R"({"frame": 24, "items": ["Cube", "Box"]})").as_object()),
        R"(timeline.set({ items: ["Cube", "Box"], frame: 24 }))");
    // a value the parameters do not name is not written, and a name is quoted as a script would
    BOOST_CHECK_EQUAL(v3d::editor::script::line("timeline::set", keyed, boost::json::parse(R"({"items": ["say \"hi\""], "x": 1})").as_object()),
        R"(timeline.set({ items: ["say \"hi\""] }))");

    const std::vector<v3d::editor::script::Parameter> others{ { "on", Type::Boolean }, { "amount", Type::Number } };
    BOOST_CHECK_EQUAL(v3d::editor::script::line("a::b", others, boost::json::parse(R"({"on": false, "amount": 0.25})").as_object()),
        "a.b({ on: false, amount: 0.25 })");
    BOOST_CHECK_EQUAL(v3d::editor::script::line("a::b", others, boost::json::parse(R"({"amount": 2.0})").as_object()), "a.b({ amount: 2 })");
}

/**
 * Parts are read from the end of their text, so an item's name may hold dots and brackets, and
 * are written back the same way.
 **/
BOOST_AUTO_TEST_CASE(parts_are_read_and_written) {
    BOOST_CHECK(v3d::editor::script::part("Cube.f[2]") == (Part{ .item = "Cube", .kind = Component::Face, .first = 2, .last = 2 }));
    BOOST_CHECK(v3d::editor::script::part("Cube.001.v[0:3]") == (Part{ .item = "Cube.001", .kind = Component::Vertex, .first = 0, .last = 3 }));
    BOOST_CHECK(v3d::editor::script::part("a[1].e[4]") == (Part{ .item = "a[1]", .kind = Component::Edge, .first = 4, .last = 4 }));
    BOOST_CHECK(v3d::editor::script::part("Path.h[7]") == (Part{ .item = "Path", .kind = Component::Handle, .first = 7, .last = 7 }));
    for (const char* text : { "Cube.f[2", "Cube.x[2]", "Cube.f[]", "Cube.f[3:1]", "Cube.f[-1]", "Cube.f[a]", "f[2]", ".f[2]", "Cube.f[1:]" }) {
        BOOST_CHECK_MESSAGE(!v3d::editor::script::part(text), text);
    }

    BOOST_CHECK_EQUAL(v3d::editor::script::text({ .item = "Cube.001", .kind = Component::Vertex, .first = 0, .last = 3 }), "Cube.001.v[0:3]");
    BOOST_CHECK_EQUAL(v3d::editor::script::text({ .item = "Cube", .kind = Component::Face, .first = 2, .last = 2 }), "Cube.f[2]");
}

/**
 * The selected parts are written in runs, each kind of each item apart.
 **/
BOOST_FIXTURE_TEST_CASE(selected_parts_are_written_in_runs, Cubes) {
    for (const unsigned int face : { 0U, 1U, 2U, 4U }) {
        scene.components().select(cube->id(), Component::Face, face, true);
    }
    scene.components().select(box->id(), Component::Vertex, 5, true);
    const std::vector<std::string> parts = v3d::editor::script::selectedParts(scene, { cube, box });
    BOOST_CHECK(parts == (std::vector<std::string>{ "Cube.f[0:2]", "Cube.f[4]", "Box.v[5]" }));
}

/**
 * Items are found by name, each once, and a name no item has is refused.
 **/
BOOST_FIXTURE_TEST_CASE(items_are_found_by_name, Cubes) {
    v3d::editor::scene::Items found;
    BOOST_CHECK(!v3d::editor::script::resolve(scene, { "Box", "Cube", "Box" }, &found));
    BOOST_REQUIRE_EQUAL(found.size(), 2U);
    BOOST_CHECK(found[0] == box && found[1] == cube);
    BOOST_CHECK(v3d::editor::script::names(found) == (std::vector<std::string>{ "Box", "Cube" }));

    BOOST_CHECK_EQUAL(v3d::editor::script::resolve(scene, { "Cube", "Sphere" }, &found).value_or(""), "there is no item called Sphere");
    BOOST_CHECK_EQUAL(found.size(), 2U);
}

/**
 * Selecting parts by name selects them and their items, and nothing else.
 **/
BOOST_FIXTURE_TEST_CASE(parts_named_are_selected, Cubes) {
    box->selected(true);
    scene.components().select(box->id(), Component::Face, 0, true);

    BOOST_CHECK(!v3d::editor::script::select(&scene, { "Cube.e[1:2]" }));
    BOOST_CHECK(cube->selected());
    BOOST_CHECK(!box->selected());
    BOOST_CHECK(!scene.components().any(box->id()));
    BOOST_CHECK(scene.components().of(cube->id(), Component::Edge) == (std::vector<v3d::brep::Index>{ 1, 2 }));
}

/**
 * Parts that cannot be selected are refused, saying why, and the selection is left as it was.
 **/
BOOST_FIXTURE_TEST_CASE(parts_that_cannot_be_selected_are_refused, Cubes) {
    scene.components().select(box->id(), Component::Face, 0, true);
    const auto refused = [this](const std::vector<std::string>& parts) {
        return v3d::editor::script::select(&scene, parts).value_or("");
    };
    BOOST_CHECK_EQUAL(refused({ "Cube" }), "Cube does not name parts, as Cube.f[2] or Cube.v[0:3] does");
    BOOST_CHECK_EQUAL(refused({ "Sphere.f[0]" }), "there is no item called Sphere");
    BOOST_CHECK_EQUAL(refused({ "Cube.h[0]" }), "Cube has no parts of the kind Cube.h[0] names");
    BOOST_CHECK_EQUAL(refused({ "Cube.f[0]", "Cube.f[5:6]" }), "Cube has no part 6 of the kind Cube.f[5:6] names");
    BOOST_CHECK(scene.components().of(box->id(), Component::Face) == (std::vector<v3d::brep::Index>{ 0 }));
    BOOST_CHECK(!scene.components().any(cube->id()));
}

/**
 * A curve's control points are its vertices, and only a Bezier point has handles.
 **/
BOOST_AUTO_TEST_CASE(a_curve_names_its_points_and_handles) {
    v3d::editor::scene::Scene scene;
    const boost::shared_ptr<v3d::curve::Curves> curves = boost::make_shared<v3d::curve::Curves>();
    BOOST_REQUIRE(curves->add({ .shape = v3d::curve::Poly{ { { 0.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F } } } }).empty());
    const boost::shared_ptr<v3d::editor::scene::item::Curve> path = boost::make_shared<v3d::editor::scene::item::Curve>(curves);
    path->name("Path");
    scene.add(path);
    BOOST_CHECK(!v3d::editor::script::select(&scene, { "Path.v[0:1]" }));
    BOOST_CHECK_EQUAL(v3d::editor::script::select(&scene, { "Path.v[2]" }).value_or(""), "Path has no part 2 of the kind Path.v[2] names");
    BOOST_CHECK_EQUAL(v3d::editor::script::select(&scene, { "Path.h[0]" }).value_or(""), "Path has no part 0 of the kind Path.h[0] names");
    BOOST_CHECK_EQUAL(v3d::editor::script::select(&scene, { "Path.f[0]" }).value_or(""), "Path has no parts of the kind Path.f[0] names");
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
