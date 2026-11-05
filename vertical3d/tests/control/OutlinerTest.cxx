/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/Primitives.h>
#include <api/render/realtime/resource/Handle.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Image.h>
#include <api/ui/Immediate.h>
#include <api/ui/tests/Typeset.h>
#include <vertical3d/src/control/Outliner.h>
#include <vertical3d/src/scene/item/Group.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/item/Light.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/Scene.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::control::Outliner;

/**
 * A scene of a rig holding an arm, which holds a hand, beside a cube and a lamp at the top.
 **/
struct Rig {
    Rig() :
        rig(named<v3d::editor::scene::item::Group>("Rig")),
        arm(named<v3d::editor::scene::item::Group>("Arm")),
        hand(named<v3d::editor::scene::item::Group>("Hand")),
        cube(boost::make_shared<v3d::editor::scene::item::Mesh>(v3d::brep::cube())),
        lamp(named<v3d::editor::scene::item::Light>("Lamp")) {
        cube->name("Cube");
        for (const boost::shared_ptr<v3d::editor::scene::item::Item>& item : v3d::editor::scene::Items{ rig, cube, arm, hand, lamp }) {
            scene.add(item);
        }
        BOOST_REQUIRE(scene.parent(arm->id(), rig->id()));
        BOOST_REQUIRE(scene.parent(hand->id(), arm->id()));
    }

    template <typename T>
    static boost::shared_ptr<v3d::editor::scene::item::Item> named(const std::string& name) {
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

    v3d::editor::scene::Scene scene;
    boost::shared_ptr<v3d::editor::scene::item::Item> rig;
    boost::shared_ptr<v3d::editor::scene::item::Item> arm;
    boost::shared_ptr<v3d::editor::scene::item::Item> hand;
    boost::shared_ptr<v3d::editor::scene::item::Mesh> cube;
    boost::shared_ptr<v3d::editor::scene::item::Item> lamp;
};

/**
 * What the outliner wrote, and where.
 **/
using Written = v3d::ui::typeset::Written;

/**
 * An outliner drawn into a 200 by 300 region at the canvas's corner, through a layer that
 * measures ten pixels a character and records what it writes.
 **/
struct Drawn : Rig {
    Drawn() :
        layer(v3d::ui::typeset::mono(&written)) {
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
    v3d::editor::scene::item::Group group;
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
 * Every item sits under a Scene row. Its mark folds them all, and a click on it selects nothing.
 **/
BOOST_FIXTURE_TEST_CASE(the_scene_row_holds_every_item, Drawn) {
    lamp->selected(true);
    frame({});
    const std::optional<glm::vec2> onScene = at("Scene");
    const std::optional<glm::vec2> onRig = at("Rig");
    BOOST_REQUIRE(onScene.has_value());
    BOOST_REQUIRE(onRig.has_value());
    const glm::vec2 sceneRow = onScene.value_or(glm::vec2(0.0F));
    const glm::vec2 rigRow = onRig.value_or(glm::vec2(0.0F));
    BOOST_CHECK_LT(sceneRow.y, rigRow.y);
    BOOST_CHECK_LT(sceneRow.x, rigRow.x);

    click(sceneRow);
    BOOST_CHECK(lamp->selected());
    const std::optional<glm::vec2> onMark = at("Scene", -6.0F);
    BOOST_REQUIRE(onMark.has_value());
    click(onMark.value_or(glm::vec2(0.0F)));
    frame({});
    BOOST_CHECK(at("Scene"));
    BOOST_CHECK(!at("Rig"));
    BOOST_CHECK(!at("Lamp"));
}

/**
 * Each kind of item has its own icon and colour, and with icons to draw, each row draws its
 * kind's icon in its kind's colour, and the filter draws a magnifier.
 **/
BOOST_FIXTURE_TEST_CASE(a_row_shows_its_kinds_icon, Drawn) {
    std::set<std::string> icons;
    std::set<std::tuple<float, float, float>> colours;
    for (const v3d::editor::scene::item::Item::Kind kind : { v3d::editor::scene::item::Item::Kind::Mesh, v3d::editor::scene::item::Item::Kind::Camera,
             v3d::editor::scene::item::Item::Kind::Light, v3d::editor::scene::item::Item::Kind::Group, v3d::editor::scene::item::Item::Kind::Curve }) {
        const Outliner::Mark marked = Outliner::mark(kind);
        icons.insert(marked.icon);
        colours.insert({ marked.colour.r, marked.colour.g, marked.colour.b });
    }
    BOOST_CHECK_EQUAL(icons.size(), 5U);
    BOOST_CHECK_EQUAL(colours.size(), 5U);

    std::vector<std::string> asked;
    outliner.icons([&asked](const std::string& source) {
        asked.push_back(source);
        return v3d::ui::Image(v3d::render::realtime::resource::TextureHandle(9));
    });
    frame({});
    BOOST_CHECK(std::ranges::find(asked, "icon:item-scene") != asked.end());
    BOOST_CHECK(std::ranges::find(asked, Outliner::mark(v3d::editor::scene::item::Item::Kind::Mesh).icon) != asked.end());
    const glm::vec4 mesh = Outliner::mark(v3d::editor::scene::item::Item::Kind::Mesh).colour;
    BOOST_CHECK(std::ranges::any_of(canvas.vertices(), [&mesh](const auto& vertex) { return vertex.colour == mesh; }));
    // the filter leads with the magnifier, in the placeholder's colour
    BOOST_CHECK(std::ranges::find(asked, "icon:search") != asked.end());
    const glm::vec4 dim = layer.dressing().placeholder;
    BOOST_CHECK(std::ranges::any_of(canvas.vertices(), [&dim](const auto& vertex) { return vertex.colour == dim; }));
}

/**
 * Each item's row ends in an eye, and a click on it is handed on and selects nothing. A row whose
 * item is hidden, by its own flag or a parent's, is dimmed.
 **/
BOOST_FIXTURE_TEST_CASE(a_click_on_an_eye_is_handed_on, Drawn) {
    std::vector<std::string> asked;
    outliner.icons([&asked](const std::string& source) {
        asked.push_back(source);
        return v3d::ui::Image(v3d::render::realtime::resource::TextureHandle(9));
    });
    std::vector<boost::shared_ptr<v3d::editor::scene::item::Item>> eyed;
    outliner.eye([&eyed](const boost::shared_ptr<v3d::editor::scene::item::Item>& item) { eyed.push_back(item); });
    rig->shown({ .views = false, .renders = true });
    frame({});
    BOOST_CHECK(std::ranges::find(asked, "icon:eye") != asked.end());
    BOOST_CHECK(std::ranges::find(asked, "icon:eye-closed") != asked.end());
    for (const Written& each : written) {
        if (each.text == "Rig" || each.text == "Hand") {
            BOOST_CHECK(each.colour == layer.dressing().disabledText);
        } else if (each.text == "Cube") {
            BOOST_CHECK(each.colour == layer.dressing().text);
        }
    }

    // the eye is the icon drawn in secondary text on the cube's row
    const std::optional<glm::vec2> found = at("Cube");
    BOOST_REQUIRE(found);
    const glm::vec2 onCube = found.value_or(glm::vec2(0.0F));
    const glm::vec4 dim = layer.dressing().secondaryText;
    float eye = 0.0F;
    bool seen = false;
    for (const auto& vertex : canvas.vertices()) {
        if (vertex.colour == dim && std::abs(vertex.position.y - onCube.y) < layer.dressing().lineHeight && (!seen || vertex.position.x < eye)) {
            eye = vertex.position.x;
            seen = true;
        }
    }
    BOOST_REQUIRE(seen);
    click(glm::vec2(eye + 2.0F, onCube.y));
    BOOST_REQUIRE_EQUAL(eyed.size(), 1U);
    BOOST_CHECK(eyed[0] == cube);
    BOOST_CHECK(!cube->selected());
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
