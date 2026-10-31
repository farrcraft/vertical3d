/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <vertical3d/src/control/Workspaces.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

namespace {

using v3d::editor::Workspaces;

/**
 * A layout of one viewport, named.
 **/
std::string single(const std::string& name, const std::string& camera) {
    return R"({"name": ")" + name + R"(", "root": {"editors": [{"kind": "viewport", "camera": ")" + camera + R"("}]}})";
}

boost::json::object parse(const std::string& text) {
    return boost::json::parse(text).as_object();
}

/**
 * The shipped set: Modelling and Animation.
 **/
struct Shipped {
    Shipped() : workspaces(boost::make_shared<v3d::log::Logger>()) {
        BOOST_REQUIRE(workspaces.load(parse(R"({"workspaces": [)" + single("Modelling", "Top") + "," + single("Animation", "Front") + "]}")));
    }

    std::string camera(std::size_t index) const {
        return std::string(workspaces.all()[index].at("root").at("editors").at(0).at("camera").as_string());
    }

    Workspaces workspaces;
};

BOOST_AUTO_TEST_SUITE(workspaces_test)

/**
 * The shipped set is what a new user starts with, and nothing of it is saved.
 **/
BOOST_FIXTURE_TEST_CASE(a_new_user_starts_from_the_shipped_set, Shipped) {
    BOOST_REQUIRE_EQUAL(workspaces.all().size(), 2U);
    BOOST_CHECK_EQUAL(workspaces.find("Animation"), 1U);
    BOOST_CHECK_EQUAL(workspaces.find("Sculpting"), 2U);
    BOOST_CHECK(!workspaces.saved());
}

/**
 * A workspace that cannot be read, has no name, or repeats a name is left out.
 **/
BOOST_AUTO_TEST_CASE(a_bad_workspace_is_left_out) {
    Workspaces workspaces(boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(workspaces.load(parse(R"({"workspaces": [)" + single("Modelling", "Top") + R"(, {"root": {"editors": []}},)"
        R"({"root": {"editors": [{"kind": "viewport"}]}},)" + single("Modelling", "Front") + ", 3]}")));
    BOOST_CHECK_EQUAL(workspaces.all().size(), 1U);
    BOOST_CHECK(!workspaces.load(parse(R"({"layout": {}})")));
}

/**
 * Keeping a workspace replaces the one of its name, or adds one. The user then has a set of
 * their own, and that set is saved and restored. Resetting puts back what was shipped.
 **/
BOOST_FIXTURE_TEST_CASE(a_kept_workspace_makes_a_set_of_the_users_own, Shipped) {
    BOOST_REQUIRE(workspaces.keep(parse(single("Modelling", "Perspective"))));
    BOOST_REQUIRE(workspaces.keep(parse(single("Shading", "Left"))));
    BOOST_CHECK(!workspaces.keep(parse(R"({"root": {"editors": [{"kind": "viewport"}]}})")));
    BOOST_REQUIRE_EQUAL(workspaces.all().size(), 3U);
    BOOST_CHECK_EQUAL(camera(0), "Perspective");
    const std::optional<boost::json::value> saved = workspaces.saved();
    BOOST_REQUIRE(saved.has_value());

    Workspaces later(boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(later.load(parse(R"({"workspaces": [)" + single("Modelling", "Top") + "]}")));
    const boost::json::value set = saved.value_or(boost::json::value());
    later.restore({ .saved = &set });
    BOOST_REQUIRE_EQUAL(later.all().size(), 3U);
    BOOST_CHECK_EQUAL(later.find("Shading"), 2U);

    BOOST_REQUIRE(workspaces.reset("Modelling"));
    BOOST_CHECK_EQUAL(camera(0), "Top");
    BOOST_CHECK(!workspaces.reset("Shading"));
}

/**
 * A project's record holds the workspace it was last shown in and its arrangement in each, and
 * survives being saved and restored.
 **/
BOOST_FIXTURE_TEST_CASE(a_project_remembers_its_arrangements, Shipped) {
    workspaces.remember({ .project = "C:/work/ship", .workspace = "Modelling" }, parse(single("Modelling", "Left")));
    workspaces.remember({ .project = "C:/work/ship", .workspace = "Animation" }, parse(single("Animation", "Top")));
    std::optional<Workspaces::Record> record = workspaces.recall("C:/work/ship");
    BOOST_REQUIRE(record.has_value());
    BOOST_CHECK_EQUAL(record.value_or(Workspaces::Record()).workspace, "Animation");
    BOOST_CHECK_EQUAL(record.value_or(Workspaces::Record()).layouts.size(), 2U);
    BOOST_CHECK(!workspaces.recall("C:/work/other").has_value());

    workspaces.forget({ .project = "C:/work/ship", .workspace = "Modelling" });
    BOOST_CHECK_EQUAL(workspaces.recall("C:/work/ship").value_or(Workspaces::Record()).layouts.size(), 1U);

    Workspaces later(boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(later.load(parse(R"({"workspaces": [)" + single("Modelling", "Top") + "]}")));
    const boost::json::value projects = workspaces.projects();
    later.restore({ .projects = &projects });
    record = later.recall("C:/work/ship");
    BOOST_REQUIRE(record.has_value());
    const Workspaces::Record kept = record.value_or(Workspaces::Record());
    BOOST_CHECK_EQUAL(kept.workspace, "Animation");
    BOOST_REQUIRE_EQUAL(kept.layouts.count("Animation"), 1U);
    BOOST_CHECK(!later.saved());
}

/**
 * A record whose project no longer exists is dropped.
 **/
BOOST_FIXTURE_TEST_CASE(records_of_gone_projects_are_pruned, Shipped) {
    workspaces.remember({ .project = "C:/work/ship", .workspace = "Modelling" }, parse(single("Modelling", "Left")));
    workspaces.remember({ .project = "C:/work/gone", .workspace = "Modelling" }, parse(single("Modelling", "Left")));
    BOOST_CHECK_EQUAL(workspaces.prune([](const std::string& project) { return project == "C:/work/ship"; }), 1U);
    BOOST_CHECK(workspaces.recall("C:/work/ship").has_value());
    BOOST_CHECK(!workspaces.recall("C:/work/gone").has_value());
}

/**
 * An exported workspace imports as itself, kept by its name.
 **/
BOOST_FIXTURE_TEST_CASE(an_exported_workspace_imports_again, Shipped) {
    const boost::json::object file = Workspaces::exported(parse(single("Review", "Perspective")));
    const std::vector<std::string> names = workspaces.imported(file);
    BOOST_REQUIRE_EQUAL(names.size(), 1U);
    BOOST_CHECK_EQUAL(names[0], "Review");
    BOOST_CHECK_EQUAL(workspaces.find("Review"), 2U);
    BOOST_CHECK(workspaces.saved().has_value());
    BOOST_CHECK(workspaces.imported(parse(R"({"layout": {}})")).empty());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
