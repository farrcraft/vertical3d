/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Immediate.h>
#include <api/ui/tests/Typeset.h>
#include <vertical3d/src/control/RenderSettingsEditor.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <optional>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::control::RenderSettingsEditor;
using v3d::editor::scene::RenderSettings;

/**
 * A render settings editor over a new document, drawn into a 400 by 300 region at the canvas's
 * corner. The layer measures ten pixels a character and records what it writes.
 **/
struct Panel {
    Panel() :
        document(boost::make_shared<v3d::editor::document::Document>(boost::make_shared<v3d::log::Logger>(),
            boost::filesystem::temp_directory_path() / "v3d_render_settings_editor")),
        layer(v3d::ui::typeset::mono(&written)),
        editor(document, [this](const v3d::editor::document::Outcome& outcome) { reported.push_back(outcome); }) {
        canvas.resize(glm::uvec2(500, 400));
        frame(hover(glm::vec2(490.0F, 390.0F)));
    }

    void frame(const v3d::ui::Immediate::Input& input) {
        written.clear();
        layer.begin(&canvas, input);
        editor.draw(&layer, 1, glm::vec4(0.0F, 0.0F, 400.0F, 300.0F));
        layer.end();
    }

    /**
     * @return a point just inside the control whose label was written
     **/
    glm::vec2 on(const std::string& text) const {
        std::optional<glm::vec2> found;
        for (const v3d::ui::typeset::Written& each : written) {
            if (each.text == text) {
                found = each.pen;
            }
        }
        BOOST_REQUIRE(found);
        return found.value_or(glm::vec2(-1.0F)) + glm::vec2(2.0F, 2.0F);
    }

    /**
     * Press at a point, drag to another, and let go there.
     **/
    void drag(const glm::vec2& from, const glm::vec2& to) {
        frame(hover(from));
        v3d::ui::Immediate::Input input = hover(from);
        input.down = true;
        input.pressed = true;
        frame(input);
        input = hover(to);
        input.down = true;
        frame(input);
        input = hover(to);
        input.released = true;
        frame(input);
        frame(hover(to));
    }

    const RenderSettings& settings() const {
        return document->scene()->renderSettings();
    }

    static v3d::ui::Immediate::Input hover(const glm::vec2& cursor) {
        v3d::ui::Immediate::Input made;
        made.cursor = cursor;
        return made;
    }

    boost::shared_ptr<v3d::editor::document::Document> document;
    v3d::render::realtime::canvas::Canvas canvas;
    std::vector<v3d::ui::typeset::Written> written;
    std::vector<v3d::editor::document::Outcome> reported;
    v3d::ui::Immediate layer;
    RenderSettingsEditor editor;
};

BOOST_AUTO_TEST_SUITE(render_settings_editor_test)

/**
 * A drag of the width changes it when it is let go, as one change, and the document's settings
 * are untouched while it is held.
 **/
BOOST_AUTO_TEST_CASE(a_dragged_side_is_one_change) {
    Panel panel;
    const glm::vec2 from = panel.on("Width");
    panel.frame(Panel::hover(from));
    v3d::ui::Immediate::Input input = Panel::hover(from);
    input.down = true;
    input.pressed = true;
    panel.frame(input);
    input = Panel::hover(from + glm::vec2(40.0F, 0.0F));
    input.down = true;
    panel.frame(input);
    BOOST_CHECK_EQUAL(panel.settings().size.x, RenderSettings::DEFAULT_WIDTH);

    input.down = false;
    input.released = true;
    panel.frame(input);
    BOOST_CHECK_EQUAL(panel.settings().size.x, RenderSettings::DEFAULT_WIDTH + 40U);
    BOOST_CHECK_EQUAL(panel.settings().size.y, RenderSettings::DEFAULT_HEIGHT);
    BOOST_CHECK_EQUAL(panel.document->history()->undoDepth(), 1U);
}

/**
 * Samples dragged below 1 are refused, and change nothing.
 **/
BOOST_AUTO_TEST_CASE(samples_out_of_range_are_refused) {
    Panel panel;
    panel.drag(panel.on("Samples"), panel.on("Samples") - glm::vec2(100.0F, 0.0F));
    BOOST_CHECK_EQUAL(panel.settings().samples, RenderSettings().samples);
    BOOST_CHECK_EQUAL(panel.document->history()->undoDepth(), 0U);
    BOOST_REQUIRE(!panel.reported.empty());
    BOOST_CHECK(!panel.reported.back().done);
}

/**
 * A choice of hider is taken as it is made.
 **/
BOOST_AUTO_TEST_CASE(a_hider_is_chosen) {
    Panel panel;
    panel.drag(panel.on("Reyes"), panel.on("Reyes"));
    panel.drag(panel.on("Ray trace"), panel.on("Ray trace"));
    BOOST_CHECK(panel.settings().hider == RenderSettings::Hider::RayTrace);
    BOOST_CHECK_EQUAL(panel.document->history()->undoDepth(), 1U);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
