/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/Primitives.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Immediate.h>
#include <vertical3d/src/control/Outliner.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::Outliner;

/**
 * A scene of a rig holding an arm, which holds a hand, beside a cube and a lamp at the top.
 **/
struct Rig {
    Rig() :
        rig(named<v3d::editor::Group>("Rig")),
        arm(named<v3d::editor::Group>("Arm")),
        hand(named<v3d::editor::Group>("Hand")),
        cube(boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube())),
        lamp(named<v3d::editor::Light>("Lamp")) {
        cube->name("Cube");
        for (const boost::shared_ptr<v3d::editor::Item>& item : v3d::editor::Items{ rig, cube, arm, hand, lamp }) {
            scene.add(item);
        }
        BOOST_REQUIRE(scene.parent(arm->id(), rig->id()));
        BOOST_REQUIRE(scene.parent(hand->id(), arm->id()));
    }

    template <typename T>
    static boost::shared_ptr<v3d::editor::Item> named(const std::string& name) {
        const boost::shared_ptr<T> made = boost::make_shared<T>();
        made->name(name);
        return made;
    }

    /**
     * @return the label of each row, indented by two spaces a level
     **/
    std::vector<std::string> shown(const std::string& filter, const std::set<std::int64_t>& folded = {}) const {
        std::vector<std::string> made;
        for (const Outliner::Row& row : Outliner::rows(scene, filter, folded)) {
            made.push_back(std::string(static_cast<std::size_t>(row.depth) * 2, ' ') + Outliner::label(*row.item));
        }
        return made;
    }

    v3d::editor::Scene scene;
    boost::shared_ptr<v3d::editor::Item> rig;
    boost::shared_ptr<v3d::editor::Item> arm;
    boost::shared_ptr<v3d::editor::Item> hand;
    boost::shared_ptr<v3d::editor::Mesh> cube;
    boost::shared_ptr<v3d::editor::Item> lamp;
};

/**
 * What the outliner wrote, and where.
 **/
struct Written final {
    std::string text;
    glm::vec2 pen;
};

/**
 * An outliner drawn into a 200 by 300 region at the canvas's corner, through a layer that
 * measures ten pixels a character and records what it writes.
 **/
struct Drawn : Rig {
    Drawn() :
        layer([](std::string_view text) { return static_cast<float>(text.size()) * 10.0F; },
            [this](std::string_view text, const glm::vec2& pen, const glm::vec4&) { written.push_back({ std::string(text), pen }); }) {
        canvas.resize(glm::uvec2(400, 400));
    }

    /**
     * Draw one frame of the outliner under some input.
     **/
    void frame(const v3d::ui::Immediate::Input& input, bool adding = false, std::size_t editor = 1) {
        written.clear();
        layer.begin(&canvas, input);
        outliner.draw(&layer, &scene, editor, glm::vec4(0.0F, 0.0F, 200.0F, 300.0F), adding);
        layer.end();
    }

    /**
     * @return a point on the row whose label was written, at a distance from its pen
     **/
    std::optional<glm::vec2> at(const std::string& label, float along = 20.0F) const {
        for (const Written& each : written) {
            if (each.text == label) {
                return each.pen + glm::vec2(along, -4.0F);
            }
        }
        return std::nullopt;
    }

    /**
     * A click: the frame that finds the cursor, the press and the release.
     **/
    void click(const glm::vec2& point, bool adding = false) {
        v3d::ui::Immediate::Input input;
        input.cursor = point;
        frame(input, adding);
        input.down = true;
        input.pressed = true;
        frame(input, adding);
        input.down = false;
        input.pressed = false;
        input.released = true;
        frame(input, adding);
    }

    v3d::render::realtime::canvas::Canvas canvas;
    std::vector<Written> written;
    v3d::ui::Immediate layer;
    Outliner outliner;
};

BOOST_AUTO_TEST_SUITE(outliner_test)

/**
 * An item is listed by its name, or by its kind when it has none.
 **/
BOOST_AUTO_TEST_CASE(a_row_names_its_item) {
    v3d::editor::Group group;
    BOOST_CHECK_EQUAL(Outliner::label(group), "Group");
    group.name("Rig");
    BOOST_CHECK_EQUAL(Outliner::label(group), "Rig");
}

/**
 * The rows follow the hierarchy, each child beneath its parent, and siblings in the scene's
 * order. A row with children can fold, and a folded one hides everything beneath it.
 **/
BOOST_FIXTURE_TEST_CASE(the_rows_follow_the_hierarchy, Rig) {
    BOOST_TEST((shown("") == std::vector<std::string>{ "Rig", "  Arm", "    Hand", "Cube", "Lamp" }));
    const std::vector<Outliner::Row> rows = Outliner::rows(scene, "", {});
    BOOST_CHECK(rows[0].parent && rows[0].open);
    BOOST_CHECK(!rows[2].parent);

    BOOST_TEST((shown("", { arm->documentId() }) == std::vector<std::string>{ "Rig", "  Arm", "Cube", "Lamp" }));
    BOOST_CHECK(!Outliner::rows(scene, "", { arm->documentId() })[1].open);
}

/**
 * The filter keeps the items whose names hold every one of its words in any case, with their
 * ancestors, opens what it keeps, and folds nothing.
 **/
BOOST_FIXTURE_TEST_CASE(the_filter_keeps_matches_and_their_ancestors, Rig) {
    BOOST_TEST((shown("HAN", { rig->documentId() }) == std::vector<std::string>{ "Rig", "  Arm", "    Hand" }));
    BOOST_TEST((shown("a m") == std::vector<std::string>{ "Rig", "  Arm", "Lamp" }));
    BOOST_TEST(shown("crate").empty());
    for (const Outliner::Row& row : Outliner::rows(scene, "hand", {})) {
        BOOST_CHECK(!row.parent);
    }
}

/**
 * A click on a row selects its item and nothing else.
 **/
BOOST_FIXTURE_TEST_CASE(a_click_selects_one_item, Drawn) {
    lamp->selected(true);
    frame({});
    const std::optional<glm::vec2> onCube = at("Cube");
    BOOST_REQUIRE(onCube);
    click(*onCube);
    BOOST_CHECK(cube->selected());
    BOOST_CHECK(!lamp->selected());
}

/**
 * While adding, a click selects an item that is not selected, deselects one that is, and leaves
 * the rest.
 **/
BOOST_FIXTURE_TEST_CASE(adding_toggles_the_item_clicked, Drawn) {
    lamp->selected(true);
    frame({});
    const std::optional<glm::vec2> onCube = at("Cube");
    const std::optional<glm::vec2> onLamp = at("Lamp");
    BOOST_REQUIRE(onCube && onLamp);
    click(*onCube, true);
    BOOST_CHECK(cube->selected());
    BOOST_CHECK(lamp->selected());
    click(*onLamp, true);
    BOOST_CHECK(!lamp->selected());
    BOOST_CHECK(cube->selected());
}

/**
 * A click on a row's mark folds it, hiding its children, and selects nothing.
 **/
BOOST_FIXTURE_TEST_CASE(a_click_on_the_mark_folds_a_row, Drawn) {
    frame({});
    BOOST_REQUIRE(at("Hand"));
    // the mark is the line before the label
    const std::optional<glm::vec2> onMark = at("Arm", -6.0F);
    BOOST_REQUIRE(onMark);
    click(*onMark);
    frame({});
    BOOST_CHECK(!at("Hand"));
    BOOST_CHECK(at("Arm"));
    BOOST_CHECK(!arm->selected());
}

/**
 * Each outliner keeps its own folds, and one that has gone is forgotten.
 **/
BOOST_FIXTURE_TEST_CASE(each_outliner_keeps_its_own_folds, Drawn) {
    frame({});
    const std::optional<glm::vec2> onMark = at("Rig", -6.0F);
    BOOST_REQUIRE(onMark);
    click(*onMark);
    frame({}, false, 1);
    BOOST_CHECK(!at("Arm"));
    frame({}, false, 2);
    BOOST_CHECK(at("Arm"));

    outliner.keep({ 2 });
    frame({}, false, 1);
    BOOST_CHECK(at("Arm"));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
