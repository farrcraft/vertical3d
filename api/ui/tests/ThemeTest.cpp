/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/kind/Json.h>
#include <api/render/realtime/Canvas.h>
#include <api/ui/Container.h>
#include <api/ui/Engine.h>
#include <api/ui/Image.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Icon.h>
#include <api/ui/component/Label.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/paint/ComponentRenderer.h>
#include <api/ui/style/Button.h>
#include <api/ui/style/Style.h>
#include <api/ui/style/Theme.h>
#include <api/ui/style/property/Color.h>
#include <api/ui/style/property/Font.h>
#include <api/ui/style/property/Image.h>
#include <api/ui/style/property/Number.h>

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>

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

    const boost::shared_ptr<v3d::asset::kind::Json> config = boost::make_shared<v3d::asset::kind::Json>(
        "vgui", v3d::asset::Type::JsonDocument, boost::json::parse(document).as_object());
    *loaded = ui->load(config);
    return ui;
}

/**
 * A renderer that records nothing but the geometry, since none of these cases is about
 * where a label went.
 **/
v3d::ui::paint::ComponentRenderer renderer() {
    return v3d::ui::paint::ComponentRenderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * 10.0f; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});
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

/**
 * Answers the names in a sprite sheet with the one texture the sheet was uploaded to and the
 * part of it each name is, the way an app with sheets would, and nothing for anything else.
 **/
struct Sheet final {
    v3d::ui::Image operator()(const std::string& source) const {
        const auto found = regions.find(source);
        if (found == regions.end()) {
            return v3d::ui::Image();
        }
        return v3d::ui::Image(v3d::render::realtime::TextureHandle(9), found->second.first, found->second.second);
    }

    std::map<std::string, std::pair<glm::vec2, glm::vec2>> regions = {
        { "items/wood", { glm::vec2(0.0f, 0.0f), glm::vec2(0.25f, 0.5f) } },
        { "items/stone", { glm::vec2(0.25f, 0.0f), glm::vec2(0.5f, 0.5f) } },
        { "skins/center", { glm::vec2(0.5f, 0.5f), glm::vec2(0.75f, 1.0f) } },
    };
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

    const std::vector<boost::shared_ptr<v3d::ui::style::Style>> chrome = dark->getStyleSet("", "ui");
    BOOST_REQUIRE_EQUAL(chrome.size(), 1U);

    const boost::shared_ptr<v3d::ui::style::property::Color> panel =
        boost::dynamic_pointer_cast<v3d::ui::style::property::Color>(chrome.front()->property("panel", "color"));
    BOOST_REQUIRE(panel);
    BOOST_CHECK_CLOSE(panel->value().a, 0.5f, 0.001f);

    const boost::shared_ptr<v3d::ui::style::property::Number> height =
        boost::dynamic_pointer_cast<v3d::ui::style::property::Number>(chrome.front()->property("bar-height", "number"));
    BOOST_REQUIRE(height);
    BOOST_CHECK_CLOSE(height->value(), 40.0f, 0.001f);

    const std::vector<boost::shared_ptr<v3d::ui::style::Style>> labels = dark->getStyleSet("", "label");
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

    const std::vector<boost::shared_ptr<v3d::ui::style::Style>> buttons = ui->theme("dark")->getStyleSet("", "button");
    BOOST_REQUIRE_EQUAL(buttons.size(), 1U);

    const boost::shared_ptr<v3d::ui::style::Button> styled =
        boost::dynamic_pointer_cast<v3d::ui::style::Button>(buttons.front());
    BOOST_REQUIRE(styled);
    BOOST_CHECK((styled->state() == v3d::ui::style::Button::State::Normal));

    const boost::shared_ptr<v3d::ui::style::property::Image> corner =
        boost::dynamic_pointer_cast<v3d::ui::style::property::Image>(styled->property("top-left", "image"));
    BOOST_REQUIRE(corner);
    BOOST_CHECK_EQUAL(corner->source(), "skins/tl.tga");
    BOOST_CHECK((corner->align() == v3d::ui::style::Property::TOP_LEFT));
    // nothing has uploaded it, so the handle it will draw with is unset
    BOOST_CHECK(!corner->image().valid());
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

    v3d::ui::paint::ComponentRenderer drawing = renderer();
    const v3d::ui::paint::Dressing defaults;

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

    v3d::ui::paint::ComponentRenderer drawing = renderer();
    const v3d::ui::paint::Dressing defaults;
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
    BOOST_CHECK(centre->image().valid());

    const boost::shared_ptr<v3d::ui::component::Icon> icon =
        boost::dynamic_pointer_cast<v3d::ui::component::Icon>(ui->container("hud")->get("logo"));
    BOOST_REQUIRE(icon);
    BOOST_CHECK(icon->image().valid());
}

/**
 * An icon inside a panel inside a box is resolved, which is where an icon in a real
 * document actually is.
 *
 * Loader hands a nested component to its parent rather than to the container, so the
 * container's own list reaches only what nothing laid out. Container::get searches the whole
 * tree and the image pass has to as well, or the two disagree about what a document holds.
 * The case earns its place because every image in this tree's own documents is a toolbar's,
 * and a strip's buttons are held separately - so nothing here exercises the ordinary shape.
 **/
BOOST_AUTO_TEST_CASE(an_image_nested_in_a_layout_is_resolved) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Engine> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "panel", "name": "frame", "children": [
                { "type": "icon", "name": "mark", "source": "art/mark.png" },
                { "type": "hbox", "name": "hotbar", "children": [
                    { "type": "icon", "name": "slot-one", "source": "art/slot.png" },
                    { "type": "button", "name": "use", "label": "Use", "icon": "art/use.png" }
                ] }
            ] }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);

    Uploader uploader;
    BOOST_CHECK_EQUAL(ui->resolveImages([&uploader](const std::string& source) { return uploader(source); }), 3U);
    BOOST_REQUIRE_EQUAL(uploader.asked.size(), 3U);
    BOOST_CHECK_EQUAL(uploader.asked[0], "art/mark.png");
    BOOST_CHECK_EQUAL(uploader.asked[1], "art/slot.png");
    BOOST_CHECK_EQUAL(uploader.asked[2], "art/use.png");

    const boost::shared_ptr<v3d::ui::component::Icon> deepest =
        boost::dynamic_pointer_cast<v3d::ui::component::Icon>(ui->container("hud")->get("slot-one"));
    BOOST_REQUIRE(deepest);
    BOOST_CHECK_EQUAL(deepest->image().texture.id(), 7U);
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
    BOOST_CHECK(!icon->image().valid());
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

    v3d::ui::paint::ComponentRenderer drawing = renderer();
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
 * A theme's "inactive" style is what a button that cannot be used is skinned from, and it is
 * chosen by Component::enabled() rather than by a button state - which is the route that did
 * not exist before ADR-0059, when nothing anywhere read the style the loader had parsed.
 **/
BOOST_AUTO_TEST_CASE(a_disabled_button_is_drawn_from_the_inactive_style) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Engine> ui = load(R"({
        "themes": [ { "name": "dark", "styles": [
            {
                "class": "button", "name": "default", "state": "inactive",
                "images": [ { "name": "center", "source": "skins/spent.tga" } ]
            } ] } ],
        "containers": [ { "name": "hud", "visible": true, "components": [] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);
    ui->resolveImages([](const std::string&) { return v3d::render::realtime::TextureHandle(3); });

    v3d::ui::paint::ComponentRenderer drawing = renderer();
    drawing.theme(ui->theme("dark"));

    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);

    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::make_shared<v3d::ui::component::Button>();
    button->label("Continue");
    button->position(glm::vec2(10.0f, 20.0f));
    button->size(glm::vec2(100.0f, 30.0f));

    // an enabled button takes the normal style, which this theme does not carry
    drawing.draw(&canvas, button);
    BOOST_CHECK(canvas.empty());

    button->enabled(false);
    drawing.draw(&canvas, button);
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 4U);
    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 1U);
    BOOST_CHECK(canvas.batches().front().texture.valid());
}

/**
 * A button whose theme names no images for its state is the flat one the toolbars draw, and
 * an unlit flat button is only its label.
 **/
BOOST_AUTO_TEST_CASE(a_button_with_no_skin_is_drawn_flat) {
    v3d::ui::paint::ComponentRenderer drawing = renderer();
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
    v3d::ui::paint::ComponentRenderer drawing = renderer();
    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);

    const boost::shared_ptr<v3d::ui::component::Icon> icon =
        boost::make_shared<v3d::ui::component::Icon>("skins/logo.tga");
    icon->position(glm::vec2(4.0f, 8.0f));
    icon->size(glm::vec2(32.0f, 16.0f));

    drawing.draw(&canvas, icon);
    BOOST_CHECK(canvas.empty());

    icon->image(v3d::render::realtime::TextureHandle(5));
    drawing.draw(&canvas, icon);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4U);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.x, 4.0f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[2].position.y, 24.0f, 0.001f);
    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 1U);
    BOOST_CHECK_EQUAL(canvas.batches().front().texture.id(), 5U);
}

/**
 * A resolver can answer with part of a texture, and an icon, a button and a skin all draw
 * that part rather than the whole sheet it is on.
 **/
BOOST_AUTO_TEST_CASE(an_image_resolved_to_part_of_a_sheet_draws_that_part) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Engine> ui = load(R"({
        "themes": [ { "name": "dark", "styles": [
            {
                "class": "button", "name": "default", "state": "normal",
                "images": [ { "name": "center", "source": "skins/center" } ]
            } ] } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "icon", "name": "slot", "source": "items/wood" },
            { "type": "button", "name": "use", "label": "Use", "icon": "items/stone" }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);
    BOOST_CHECK_EQUAL(ui->resolveImages(Sheet()), 3U);

    v3d::ui::paint::ComponentRenderer drawing = renderer();
    drawing.theme(ui->theme("dark"));
    drawing.dressing().iconSize = 20.0f;

    const boost::shared_ptr<v3d::ui::component::Icon> icon =
        boost::dynamic_pointer_cast<v3d::ui::component::Icon>(ui->container("hud")->get("slot"));
    BOOST_REQUIRE(icon);
    icon->size(glm::vec2(32.0f, 32.0f));
    v3d::render::realtime::Canvas iconCanvas;
    iconCanvas.resize(800, 600);
    drawing.draw(&iconCanvas, icon);
    BOOST_REQUIRE_EQUAL(iconCanvas.vertices().size(), 4U);
    BOOST_CHECK_EQUAL(iconCanvas.batches().front().texture.id(), 9U);
    BOOST_CHECK(iconCanvas.vertices()[0].uv == glm::vec2(0.0f, 0.0f));
    BOOST_CHECK(iconCanvas.vertices()[2].uv == glm::vec2(0.25f, 0.5f));

    // the skin's centre is the first quad and the button's icon the one drawn over it
    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::dynamic_pointer_cast<v3d::ui::component::Button>(ui->container("hud")->get("use"));
    BOOST_REQUIRE(button);
    button->size(glm::vec2(100.0f, 30.0f));
    v3d::render::realtime::Canvas buttonCanvas;
    buttonCanvas.resize(800, 600);
    drawing.draw(&buttonCanvas, button);
    BOOST_REQUIRE_EQUAL(buttonCanvas.vertices().size(), 2U * 4U);
    BOOST_CHECK(buttonCanvas.vertices()[0].uv == glm::vec2(0.5f, 0.5f));
    BOOST_CHECK(buttonCanvas.vertices()[2].uv == glm::vec2(0.75f, 1.0f));
    BOOST_CHECK(buttonCanvas.vertices()[4].uv == glm::vec2(0.25f, 0.0f));
    BOOST_CHECK(buttonCanvas.vertices()[6].uv == glm::vec2(0.5f, 0.5f));
}

/**
 * An icon pointed at a different source shows nothing until that source is resolved, and
 * then keeps showing it however many times the whole ui is resolved again - which is what
 * lets a grid of cells change what they hold and survive a reload.
 **/
BOOST_AUTO_TEST_CASE(an_icon_given_a_new_source_keeps_it_across_a_resolve) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Engine> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "icon", "name": "slot", "source": "items/wood" }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);
    const Sheet sheet;
    ui->resolveImages(sheet);

    const boost::shared_ptr<v3d::ui::component::Icon> icon =
        boost::dynamic_pointer_cast<v3d::ui::component::Icon>(ui->container("hud")->get("slot"));
    BOOST_REQUIRE(icon);
    BOOST_CHECK(icon->image().uv1 == glm::vec2(0.25f, 0.5f));

    // naming the same source again keeps what it resolved to
    icon->source("items/wood");
    BOOST_CHECK(icon->image().valid());

    icon->source("items/stone");
    BOOST_CHECK(!icon->image().valid());

    BOOST_CHECK_EQUAL(ui->resolveComponentImages(sheet, icon), 1U);
    BOOST_CHECK(icon->image().uv0 == glm::vec2(0.25f, 0.0f));

    ui->resolveImages(sheet);
    BOOST_CHECK_EQUAL(icon->source(), "items/stone");
    BOOST_CHECK(icon->image().uv0 == glm::vec2(0.25f, 0.0f));
    BOOST_CHECK(icon->image().uv1 == glm::vec2(0.5f, 0.5f));
}

/**
 * A button pointed at a different icon falls back to its label until the new one is
 * resolved, rather than drawing the old picture under the new name.
 **/
BOOST_AUTO_TEST_CASE(a_button_given_a_new_icon_drops_the_old_image) {
    v3d::ui::component::Button button;
    button.icon("items/wood");
    button.image(Sheet()("items/wood"));
    BOOST_REQUIRE(button.image().valid());

    button.icon("items/wood");
    BOOST_CHECK(button.image().valid());

    button.icon("items/stone");
    BOOST_CHECK(!button.image().valid());
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
    v3d::ui::paint::ComponentRenderer drawing(
        [](std::string_view text) { return static_cast<float>(text.size()) * 10.0f; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            written.push_back(Written{ std::string(text), pen, colour });
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
    const v3d::ui::paint::Dressing& dressing = drawing.dressing();
    BOOST_CHECK_CLOSE(bar->bound().size().x, dressing.iconSize + dressing.padding, 0.001f);
    BOOST_CHECK_CLOSE(drawing.insets(*ui).x, dressing.iconSize + dressing.padding + 1.0f, 0.001f);

    // and the icon is centred in the button's own box
    const v3d::type::geometry::Bound2D box = bar->button(0)->bound();
    BOOST_CHECK_CLOSE(canvas.vertices()[8].position.x,
        box.position().x + (box.size().x - dressing.iconSize) * 0.5f, 0.001f);
}

/**
 * A button whose icon was never resolved falls back to its label, which is what keeps a
 * missing image from leaving an unreadable strip.
 **/
BOOST_AUTO_TEST_CASE(an_unresolved_icon_leaves_the_label_drawn) {
    std::vector<Written> written;
    v3d::ui::paint::ComponentRenderer drawing(
        [](std::string_view text) { return static_cast<float>(text.size()) * 10.0f; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            written.push_back(Written{ std::string(text), pen, colour });
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

    v3d::ui::paint::ComponentRenderer drawing = renderer();
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
