/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <cstddef>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "../ComponentRenderer.h"
#include "../../render/realtime/Canvas.h"
#include "../Container.h"
#include "../Style.h"
#include "../style/Theme.h"
#include <entt/entt.hpp>
#include "../Engine.h"
#include "../style/Button.h"
#include "../style/property/Color.h"
#include "../style/property/Font.h"
#include "../style/property/Image.h"
#include "../style/property/Number.h"

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * A ui engine over a document written inline, which is what a config file amounts to by
 * the time it reaches the loader.
 **/
boost::shared_ptr<v3d::ui::Engine> load(const std::string& document, bool* loaded) {
    boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    boost::shared_ptr<v3d::ui::Engine> ui = boost::make_shared<v3d::ui::Engine>(
        boost::make_shared<v3d::event::Engine>(dispatcher),
        dispatcher,
        boost::make_shared<v3d::log::Logger>());

    const boost::shared_ptr<v3d::asset::Json> config = boost::make_shared<v3d::asset::Json>(
        "vgui", v3d::asset::Type::JsonDocument, boost::json::parse(document).as_object());
    *loaded = ui->load(config);
    return ui;
}

/**
 * A renderer that records nothing but the geometry, since none of these cases is about
 * where a label went.
 **/
v3d::ui::ComponentRenderer renderer() {
    return v3d::ui::ComponentRenderer(
        [](const std::string& text) { return static_cast<float>(text.size()) * 10.0f; },
        [](const std::string&, const glm::vec2&, const glm::vec4&) {});
}

/**
 * What the renderer asked to be written, for the cases that are about a label.
 **/
struct Written final {
    std::string text;
    glm::vec2 pen;
    glm::vec4 colour;
};

/**
 * Hands every source the same texture, and says which sources it was asked for.
 **/
struct Uploader final {
    v3d::render::realtime::TextureHandle operator()(const std::string& source) {
        asked.push_back(source);
        return v3d::render::realtime::TextureHandle(7);
    }

    std::vector<std::string> asked;
};

const char* const themedDocument = R"({
        "themes": [
            {
                "name": "dark",
                "styles": [
                    {
                        "class": "ui", "name": "default",
                        "colors": [ { "name": "panel", "value": [0.1, 0.2, 0.3, 0.5] } ],
                        "numbers": [ { "name": "bar-height", "value": 40 } ]
                    },
                    {
                        "class": "button", "name": "default", "state": "normal",
                        "numbers": [ { "name": "corner", "value": 4 } ],
                        "images": [
                            { "name": "center", "source": "skins/center.tga" },
                            { "name": "top-left", "source": "skins/tl.tga", "align": "top-left" }
                        ]
                    },
                    {
                        "class": "label", "name": "default",
                        "fonts": [ { "name": "label", "face": "Vera", "size": 18, "bold": true } ]
                    }
                ]
            },
            { "name": "light" }
        ],
        "theme": "light",
        "containers": [ { "name": "hud", "visible": true, "components": [] } ]
})";

};  // namespace

BOOST_AUTO_TEST_SUITE(theme_test)

/**
 * A theme carries styles, a style carries properties of four kinds, and each is read as the
 * kind of the array it was written in.
 **/
BOOST_AUTO_TEST_CASE(a_theme_loads_its_styles_and_their_properties) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Engine> ui = load(themedDocument, &loaded);
    BOOST_REQUIRE(loaded);

    const boost::shared_ptr<v3d::ui::style::Theme> dark = ui->theme("dark");
    BOOST_REQUIRE(dark);
    BOOST_CHECK_EQUAL(dark->getStyleSet("", "").size(), 3U);

    const std::vector<boost::shared_ptr<v3d::ui::Style>> chrome = dark->getStyleSet("", "ui");
    BOOST_REQUIRE_EQUAL(chrome.size(), 1U);

    const boost::shared_ptr<v3d::ui::style::property::Color> panel =
        boost::dynamic_pointer_cast<v3d::ui::style::property::Color>(chrome.front()->property("panel", "color"));
    BOOST_REQUIRE(panel);
    BOOST_CHECK_CLOSE(panel->value().a, 0.5f, 0.001f);

    const boost::shared_ptr<v3d::ui::style::property::Number> height =
        boost::dynamic_pointer_cast<v3d::ui::style::property::Number>(chrome.front()->property("bar-height", "number"));
    BOOST_REQUIRE(height);
    BOOST_CHECK_CLOSE(height->value(), 40.0f, 0.001f);

    const std::vector<boost::shared_ptr<v3d::ui::Style>> labels = dark->getStyleSet("", "label");
    BOOST_REQUIRE_EQUAL(labels.size(), 1U);
    const boost::shared_ptr<v3d::ui::style::property::Font> font =
        boost::dynamic_pointer_cast<v3d::ui::style::property::Font>(labels.front()->property("label", "font"));
    BOOST_REQUIRE(font);
    BOOST_CHECK_EQUAL(font->face(), "Vera");
    BOOST_CHECK_EQUAL(font->size(), 18U);
    BOOST_CHECK(font->bold());
    BOOST_CHECK(!font->italics());
}

/**
 * A button style is told apart by its state as well as by its name, and an image property
 * carries its source and its alignment.
 **/
BOOST_AUTO_TEST_CASE(a_button_style_carries_a_state_and_its_images) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Engine> ui = load(themedDocument, &loaded);
    BOOST_REQUIRE(loaded);

    const std::vector<boost::shared_ptr<v3d::ui::Style>> buttons = ui->theme("dark")->getStyleSet("", "button");
    BOOST_REQUIRE_EQUAL(buttons.size(), 1U);

    const boost::shared_ptr<v3d::ui::style::Button> styled =
        boost::dynamic_pointer_cast<v3d::ui::style::Button>(buttons.front());
    BOOST_REQUIRE(styled);
    BOOST_CHECK((styled->state() == v3d::ui::component::Button::STATE_NORMAL));

    const boost::shared_ptr<v3d::ui::style::property::Image> corner =
        boost::dynamic_pointer_cast<v3d::ui::style::property::Image>(styled->property("top-left", "image"));
    BOOST_REQUIRE(corner);
    BOOST_CHECK_EQUAL(corner->source(), "skins/tl.tga");
    BOOST_CHECK((corner->align() == v3d::ui::style::Property::TOP_LEFT));
    // nothing has uploaded it, so the handle it will draw with is unset
    BOOST_CHECK(!corner->texture().valid());
}

/**
 * The document names which theme is active. Without the field the first one loaded is,
 * which is what every config in the tree relies on.
 **/
BOOST_AUTO_TEST_CASE(the_document_names_the_active_theme) {
    bool loaded = false;
    boost::shared_ptr<v3d::ui::Engine> ui = load(themedDocument, &loaded);
    BOOST_REQUIRE(loaded);
    BOOST_REQUIRE(ui->activeTheme());
    BOOST_CHECK_EQUAL(ui->activeTheme()->name(), "light");

    ui = load(R"({ "themes": [ { "name": "first" }, { "name": "second" } ], "containers": [] })", &loaded);
    BOOST_REQUIRE(loaded);
    BOOST_REQUIRE(ui->activeTheme());
    BOOST_CHECK_EQUAL(ui->activeTheme()->name(), "first");
}

/**
 * A theme naming one that was never loaded is refused rather than leaving the ui drawn with
 * a theme the document did not ask for.
 **/
BOOST_AUTO_TEST_CASE(an_unknown_active_theme_is_refused) {
    bool loaded = true;
    load(R"({ "themes": [ { "name": "first" } ], "theme": "missing", "containers": [] })", &loaded);
    BOOST_CHECK(!loaded);
}

/**
 * Every colour and metric the theme does not name keeps the default it had, so a theme that
 * names one colour changes one thing.
 **/
BOOST_AUTO_TEST_CASE(a_theme_overrides_what_it_names_and_no_more) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Engine> ui = load(themedDocument, &loaded);
    BOOST_REQUIRE(loaded);

    v3d::ui::ComponentRenderer drawing = renderer();
    const v3d::ui::ComponentRenderer::Dressing defaults;

    drawing.theme(ui->theme("dark"));

    BOOST_CHECK_CLOSE(drawing.dressing().panel.b, 0.3f, 0.001f);
    BOOST_CHECK_CLOSE(drawing.dressing().barHeight, 40.0f, 0.001f);
    // the style named neither, so both are what they were
    BOOST_CHECK_CLOSE(drawing.dressing().lineHeight, defaults.lineHeight, 0.001f);
    BOOST_CHECK_CLOSE(drawing.dressing().border.r, defaults.border.r, 0.001f);
}

/**
 * A theme with no styles in it draws exactly as no theme at all does, which is what keeps
 * every ui config written before the styles could be read drawing the same.
 **/
BOOST_AUTO_TEST_CASE(a_nameless_theme_changes_nothing) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Engine> ui = load(
        R"({ "themes": [ { "name": "plain" } ], "containers": [] })", &loaded);
    BOOST_REQUIRE(loaded);

    v3d::ui::ComponentRenderer drawing = renderer();
    const v3d::ui::ComponentRenderer::Dressing defaults;
    drawing.theme(ui->activeTheme());

    BOOST_CHECK_CLOSE(drawing.dressing().barHeight, defaults.barHeight, 0.001f);
    BOOST_CHECK_CLOSE(drawing.dressing().panel.a, defaults.panel.a, 0.001f);
}

/**
 * The image pass hands every source the config named to the app and keeps what it gives
 * back - the images of every theme, and every icon in every container.
 **/
BOOST_AUTO_TEST_CASE(the_image_pass_resolves_every_source_the_config_named) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Engine> ui = load(R"({
        "themes": [ {
            "name": "dark",
            "styles": [ {
                "class": "button", "name": "default", "state": "normal",
                "images": [ { "name": "center", "source": "skins/center.tga" } ]
            } ]
        } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "icon", "name": "logo", "source": "skins/logo.tga" }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);

    Uploader uploader;
    const std::size_t resolved = ui->resolveImages([&uploader](const std::string& source) { return uploader(source); });

    BOOST_CHECK_EQUAL(resolved, 2U);
    BOOST_REQUIRE_EQUAL(uploader.asked.size(), 2U);
    BOOST_CHECK_EQUAL(uploader.asked[0], "skins/center.tga");
    BOOST_CHECK_EQUAL(uploader.asked[1], "skins/logo.tga");

    const boost::shared_ptr<v3d::ui::style::property::Image> centre =
        boost::dynamic_pointer_cast<v3d::ui::style::property::Image>(
            ui->theme("dark")->getStyleSet("", "button").front()->property("center", "image"));
    BOOST_REQUIRE(centre);
    BOOST_CHECK(centre->texture().valid());

    const boost::shared_ptr<v3d::ui::component::Icon> icon =
        boost::dynamic_pointer_cast<v3d::ui::component::Icon>(ui->container("hud")->get("logo"));
    BOOST_REQUIRE(icon);
    BOOST_CHECK(icon->texture().valid());
}

/**
 * A source the app cannot resolve leaves the handle unset rather than a handle to nothing,
 * and the rest of the pass carries on.
 **/
BOOST_AUTO_TEST_CASE(an_unresolved_source_leaves_the_handle_unset) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Engine> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "icon", "name": "logo", "source": "missing.tga" }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);

    const std::size_t resolved = ui->resolveImages(
        [](const std::string&) { return v3d::render::realtime::TextureHandle(); });

    BOOST_CHECK_EQUAL(resolved, 0U);
    const boost::shared_ptr<v3d::ui::component::Icon> icon =
        boost::dynamic_pointer_cast<v3d::ui::component::Icon>(ui->container("hud")->get("logo"));
    BOOST_REQUIRE(icon);
    BOOST_CHECK(!icon->texture().valid());
}

/**
 * A button, a label and an icon are components a container can hold, and each carries the
 * box it asks for. position() and size() stay empty until something draws them, per
 * ADR-0034.
 **/
BOOST_AUTO_TEST_CASE(a_container_holds_buttons_labels_and_icons) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Engine> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "button", "name": "go", "label": "Go", "position": [10, 20], "size": [100, 30],
              "style": "flat", "context": "ui", "command": "quit", "toggle": true },
            { "type": "label", "name": "score", "label": "Score", "position": [5, 5] },
            { "type": "icon", "name": "logo", "source": "skins/logo.tga", "visible": false }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);

    const boost::shared_ptr<v3d::ui::Container> container = ui->container("hud");
    BOOST_REQUIRE(container);
    BOOST_CHECK_EQUAL(container->components().size(), 3U);

    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::dynamic_pointer_cast<v3d::ui::component::Button>(container->get("go"));
    BOOST_REQUIRE(button);
    BOOST_CHECK_EQUAL(button->label(), "Go");
    BOOST_CHECK(button->toggle());
    BOOST_CHECK_EQUAL(button->style(), "flat");
    BOOST_CHECK(button->layout().x.unit() == v3d::ui::Length::Unit::Pixels);
    BOOST_CHECK_CLOSE(button->layout().x.value(), 10.0f, 0.001f);
    BOOST_CHECK_CLOSE(button->layout().height.value(), 30.0f, 0.001f);
    BOOST_CHECK_CLOSE(button->position().x, 0.0f, 0.001f);
    BOOST_CHECK_EQUAL(button->event().str(), "ui::quit");

    const boost::shared_ptr<v3d::ui::component::Label> label =
        boost::dynamic_pointer_cast<v3d::ui::component::Label>(container->get("score"));
    BOOST_REQUIRE(label);
    BOOST_CHECK_EQUAL(label->text(), "Score");

    // visible is read for a component the same way it is for a container
    BOOST_REQUIRE(container->get("logo"));
    BOOST_CHECK(!container->get("logo")->visible());
}

/**
 * A component type the library does not know is refused rather than skipped, so a typo in a
 * config is not a component silently missing from the window.
 **/
BOOST_AUTO_TEST_CASE(an_unknown_component_type_is_refused) {
    bool loaded = true;
    load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "carousel", "name": "spinner" }
        ] } ]
    })", &loaded);
    BOOST_CHECK(!loaded);
}

/**
 * A skinned button is nine quads against the images its style names, and it is drawn instead
 * of the flat highlight rather than under it. Only the two the style names are drawn.
 **/
BOOST_AUTO_TEST_CASE(a_button_is_drawn_from_the_images_its_style_names) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Engine> ui = load(themedDocument, &loaded);
    BOOST_REQUIRE(loaded);
    ui->resolveImages([](const std::string&) { return v3d::render::realtime::TextureHandle(3); });

    v3d::ui::ComponentRenderer drawing = renderer();
    drawing.theme(ui->theme("dark"));

    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);

    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::make_shared<v3d::ui::component::Button>();
    button->label("Go");
    button->position(glm::vec2(10.0f, 20.0f));
    button->size(glm::vec2(100.0f, 30.0f));
    drawing.draw(&canvas, button);

    // the centre and the top left corner, and nothing else the style did not name
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 2U * 4U);
    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 1U);
    BOOST_CHECK(canvas.batches().front().texture.valid());

    // the corner is drawn at the size the style's own number gives
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.x, 10.0f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[2].position.x, 14.0f, 0.001f);
}

/**
 * A button whose theme names no images for its state is the flat one the toolbars draw, and
 * an unlit flat button is only its label.
 **/
BOOST_AUTO_TEST_CASE(a_button_with_no_skin_is_drawn_flat) {
    v3d::ui::ComponentRenderer drawing = renderer();
    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);

    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::make_shared<v3d::ui::component::Button>();
    button->label("Go");
    button->size(glm::vec2(100.0f, 30.0f));

    drawing.draw(&canvas, button);
    BOOST_CHECK(canvas.empty());

    button->state(v3d::ui::component::Button::STATE_HOVER);
    drawing.draw(&canvas, button);
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 4U);
}

/**
 * An icon is one textured quad at the size the component was given, and one whose source was
 * never resolved draws nothing at all.
 **/
BOOST_AUTO_TEST_CASE(an_icon_draws_the_texture_it_was_resolved_to) {
    v3d::ui::ComponentRenderer drawing = renderer();
    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);

    const boost::shared_ptr<v3d::ui::component::Icon> icon =
        boost::make_shared<v3d::ui::component::Icon>("skins/logo.tga");
    icon->position(glm::vec2(4.0f, 8.0f));
    icon->size(glm::vec2(32.0f, 16.0f));

    drawing.draw(&canvas, icon);
    BOOST_CHECK(canvas.empty());

    icon->texture(v3d::render::realtime::TextureHandle(5));
    drawing.draw(&canvas, icon);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4U);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.x, 4.0f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[2].position.y, 24.0f, 0.001f);
    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 1U);
    BOOST_CHECK_EQUAL(canvas.batches().front().texture.id(), 5U);
}

/**
 * A toolbar button names an icon, the image pass resolves it, and the button draws that
 * image instead of its label.
 **/
BOOST_AUTO_TEST_CASE(a_toolbar_button_draws_the_icon_it_names) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Engine> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "editor", "visible": true, "components": [
            { "type": "toolbar", "name": "tools", "edge": "left", "buttons": [
                { "label": "Select", "icon": "icons/select.png", "toggle": true },
                { "label": "Translate", "icon": "icons/translate.png", "toggle": true }
            ] }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);

    Uploader uploader;
    BOOST_CHECK_EQUAL(ui->resolveImages([&uploader](const std::string& source) { return uploader(source); }), 2U);
    BOOST_REQUIRE_EQUAL(uploader.asked.size(), 2U);
    BOOST_CHECK_EQUAL(uploader.asked[0], "icons/select.png");

    std::vector<Written> written;
    v3d::ui::ComponentRenderer drawing(
        [](const std::string& text) { return static_cast<float>(text.size()) * 10.0f; },
        [&written](const std::string& text, const glm::vec2& pen, const glm::vec4& colour) {
            written.push_back(Written{ text, pen, colour });
        });
    drawing.dressing().iconSize = 20.0f;

    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);
    drawing.draw(&canvas, *ui);

    // the panel, the rule, and one icon per button - and not one label
    BOOST_CHECK(written.empty());
    BOOST_CHECK_EQUAL(canvas.batches().size(), 2U);

    const boost::shared_ptr<v3d::ui::component::Toolbar> bar =
        boost::dynamic_pointer_cast<v3d::ui::component::Toolbar>(ui->container("editor")->get("tools"));
    BOOST_REQUIRE(bar);

    // the column is as wide as the icon, not as the label it would otherwise draw
    const v3d::ui::ComponentRenderer::Dressing& dressing = drawing.dressing();
    BOOST_CHECK_CLOSE(bar->bound().size().x, dressing.iconSize + dressing.padding, 0.001f);
    BOOST_CHECK_CLOSE(drawing.insets(*ui).x, dressing.iconSize + dressing.padding + 1.0f, 0.001f);

    // and the icon is centred in the button's own box
    const v3d::type::Bound2D box = bar->button(0)->bound();
    BOOST_CHECK_CLOSE(canvas.vertices()[8].position.x,
        box.position().x + (box.size().x - dressing.iconSize) * 0.5f, 0.001f);
}

/**
 * A button whose icon was never resolved falls back to its label, which is what keeps a
 * missing image from leaving an unreadable strip.
 **/
BOOST_AUTO_TEST_CASE(an_unresolved_icon_leaves_the_label_drawn) {
    std::vector<Written> written;
    v3d::ui::ComponentRenderer drawing(
        [](const std::string& text) { return static_cast<float>(text.size()) * 10.0f; },
        [&written](const std::string& text, const glm::vec2& pen, const glm::vec4& colour) {
            written.push_back(Written{ text, pen, colour });
        });

    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);

    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::make_shared<v3d::ui::component::Button>();
    button->label("Select");
    button->icon("icons/select.png");
    button->size(glm::vec2(40.0f, 24.0f));
    drawing.draw(&canvas, button);

    BOOST_REQUIRE_EQUAL(written.size(), 1U);
    BOOST_CHECK_EQUAL(written.front().text, "Select");
    BOOST_CHECK(canvas.empty());
}

/**
 * A container draws what it holds, and a component that is not visible is not drawn.
 **/
BOOST_AUTO_TEST_CASE(a_container_draws_its_labels_and_icons) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Engine> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "label", "name": "score", "label": "Score", "position": [5, 5] },
            { "type": "icon", "name": "logo", "source": "skins/logo.tga", "position": [5, 40], "visible": false }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);
    ui->resolveImages([](const std::string&) { return v3d::render::realtime::TextureHandle(2); });

    v3d::ui::ComponentRenderer drawing = renderer();
    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);
    drawing.draw(&canvas, *ui);

    // the label is text, which is the app's to draw, and the hidden icon is nothing
    BOOST_CHECK(canvas.empty());

    const boost::shared_ptr<v3d::ui::component::Label> label =
        boost::dynamic_pointer_cast<v3d::ui::component::Label>(ui->container("hud")->get("score"));
    BOOST_REQUIRE(label);
    // it was drawn, so it holds the bounds it was drawn in
    BOOST_CHECK_CLOSE(label->bound().size().x, 50.0f, 0.001f);

    ui->container("hud")->get("logo")->visible(true);
    drawing.draw(&canvas, *ui);
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 4U);
}

BOOST_AUTO_TEST_SUITE_END()
