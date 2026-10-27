/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/Handle.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/type/geometry/Bound2D.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/Image.h>
#include <api/ui/Length.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Icon.h>
#include <api/ui/component/Label.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/paint/ComponentRenderer.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/style/Button.h>
#include <api/ui/style/Style.h>
#include <api/ui/style/Theme.h>
#include <api/ui/style/property/Color.h>
#include <api/ui/style/property/Image.h>
#include <api/ui/style/property/Number.h>

#include <cstddef>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <boost/make_shared.hpp>

#include "Documents.h"

namespace v3d::ui::documents {
namespace {

BOOST_AUTO_TEST_SUITE(theme_test)

/**
 * The image pass hands every source the config named to the app and keeps what it gives
 * back - the images of every theme, and every icon in every container.
 **/
BOOST_AUTO_TEST_CASE(the_image_pass_resolves_every_source_the_config_named) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
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
 * An icon inside a panel inside a box is resolved. An icon in a real document is nested
 * this way.
 *
 * Loader hands a nested component to its parent rather than to the container, so the
 * container's own list reaches only what nothing laid out. Container::get searches the whole
 * tree and the image pass has to as well, or the two disagree about what a document holds.
 * A strip's buttons are held separately from the tree, so a toolbar's images do not cover
 * this nested shape.
 **/
BOOST_AUTO_TEST_CASE(an_image_nested_in_a_layout_is_resolved) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
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
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
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
 * A skinned button is nine quads against the images its style names, and it is drawn instead
 * of the flat highlight rather than under it. Only the two the style names are drawn.
 **/
BOOST_AUTO_TEST_CASE(a_button_is_drawn_from_the_images_its_style_names) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(themedDocument, &loaded);
    BOOST_REQUIRE(loaded);
    ui->resolveImages([](const std::string&) { return v3d::render::realtime::TextureHandle(3); });

    v3d::ui::paint::ComponentRenderer drawing = renderer();
    drawing.theme(ui->theme("dark"));

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::make_shared<v3d::ui::component::Button>();
    button->label("Go");
    button->position(glm::vec2(10.0F, 20.0F));
    button->size(glm::vec2(100.0F, 30.0F));
    drawing.draw(&canvas, button);

    // the centre and the top left corner, and nothing else the style did not name
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 2U * 4U);
    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 1U);
    BOOST_CHECK(canvas.batches().front().texture.valid());

    // the corner is drawn at the size the style's own number gives
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.x, 10.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.vertices()[2].position.x, 14.0F, 0.001F);
}

/**
 * A theme's "inactive" style is what a button that cannot be used is skinned from, and it is
 * chosen by Component::enabled() rather than by a button state.
 **/
BOOST_AUTO_TEST_CASE(a_disabled_button_is_drawn_from_the_inactive_style) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
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

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::make_shared<v3d::ui::component::Button>();
    button->label("Continue");
    button->position(glm::vec2(10.0F, 20.0F));
    button->size(glm::vec2(100.0F, 30.0F));

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
    const v3d::ui::paint::ComponentRenderer drawing = renderer();
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::make_shared<v3d::ui::component::Button>();
    button->label("Go");
    button->size(glm::vec2(100.0F, 30.0F));

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
    const v3d::ui::paint::ComponentRenderer drawing = renderer();
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const boost::shared_ptr<v3d::ui::component::Icon> icon =
        boost::make_shared<v3d::ui::component::Icon>("skins/logo.tga");
    icon->position(glm::vec2(4.0F, 8.0F));
    icon->size(glm::vec2(32.0F, 16.0F));

    drawing.draw(&canvas, icon);
    BOOST_CHECK(canvas.empty());

    icon->image(v3d::render::realtime::TextureHandle(5));
    drawing.draw(&canvas, icon);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4U);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.x, 4.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.vertices()[2].position.y, 24.0F, 0.001F);
    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 1U);
    BOOST_CHECK_EQUAL(canvas.batches().front().texture.id(), 5U);
}

/**
 * A resolver can answer with part of a texture, and an icon, a button and a skin all draw
 * that part rather than the whole sheet it is on.
 **/
BOOST_AUTO_TEST_CASE(an_image_resolved_to_part_of_a_sheet_draws_that_part) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
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
    drawing.dressing().iconSize = 20.0F;

    const boost::shared_ptr<v3d::ui::component::Icon> icon =
        boost::dynamic_pointer_cast<v3d::ui::component::Icon>(ui->container("hud")->get("slot"));
    BOOST_REQUIRE(icon);
    icon->size(glm::vec2(32.0F, 32.0F));
    v3d::render::realtime::canvas::Canvas iconCanvas;
    iconCanvas.resize(glm::uvec2(800, 600));
    drawing.draw(&iconCanvas, icon);
    BOOST_REQUIRE_EQUAL(iconCanvas.vertices().size(), 4U);
    BOOST_CHECK_EQUAL(iconCanvas.batches().front().texture.id(), 9U);
    BOOST_CHECK(iconCanvas.vertices()[0].uv == glm::vec2(0.0F, 0.0F));
    BOOST_CHECK(iconCanvas.vertices()[2].uv == glm::vec2(0.25F, 0.5F));

    // the skin's centre is the first quad and the button's icon the one drawn over it
    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::dynamic_pointer_cast<v3d::ui::component::Button>(ui->container("hud")->get("use"));
    BOOST_REQUIRE(button);
    button->size(glm::vec2(100.0F, 30.0F));
    v3d::render::realtime::canvas::Canvas buttonCanvas;
    buttonCanvas.resize(glm::uvec2(800, 600));
    drawing.draw(&buttonCanvas, button);
    BOOST_REQUIRE_EQUAL(buttonCanvas.vertices().size(), 2U * 4U);
    BOOST_CHECK(buttonCanvas.vertices()[0].uv == glm::vec2(0.5F, 0.5F));
    BOOST_CHECK(buttonCanvas.vertices()[2].uv == glm::vec2(0.75F, 1.0F));
    BOOST_CHECK(buttonCanvas.vertices()[4].uv == glm::vec2(0.25F, 0.0F));
    BOOST_CHECK(buttonCanvas.vertices()[6].uv == glm::vec2(0.5F, 0.5F));
}

/**
 * An icon pointed at a different source shows nothing until that source is resolved, and
 * then keeps showing it however many times the whole ui is resolved again.
 **/
BOOST_AUTO_TEST_CASE(an_icon_given_a_new_source_keeps_it_across_a_resolve) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
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
    BOOST_CHECK(icon->image().uv1 == glm::vec2(0.25F, 0.5F));

    // naming the same source again keeps what it resolved to
    icon->source("items/wood");
    BOOST_CHECK(icon->image().valid());

    icon->source("items/stone");
    BOOST_CHECK(!icon->image().valid());

    BOOST_CHECK_EQUAL(ui->resolveComponentImages(sheet, icon), 1U);
    BOOST_CHECK(icon->image().uv0 == glm::vec2(0.25F, 0.0F));

    ui->resolveImages(sheet);
    BOOST_CHECK_EQUAL(icon->source(), "items/stone");
    BOOST_CHECK(icon->image().uv0 == glm::vec2(0.25F, 0.0F));
    BOOST_CHECK(icon->image().uv1 == glm::vec2(0.5F, 0.5F));
}

/**
 * A null entry in a container is passed over, and the icons beside it still resolve.
 **/
BOOST_AUTO_TEST_CASE(resolving_images_passes_over_a_null_component) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "icon", "name": "slot", "source": "items/wood" }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);
    ui->container("hud")->add(nullptr);

    const Sheet sheet;
    BOOST_CHECK_EQUAL(ui->resolveImages(sheet), 1U);
    BOOST_CHECK_EQUAL(ui->resolveComponentImages(sheet, nullptr), 0U);
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
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
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
        [](std::string_view text) { return static_cast<float>(text.size()) * 10.0F; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            written.push_back(Written{ std::string(text), pen, colour });
        });
    drawing.dressing().iconSize = 20.0F;

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));
    drawing.draw(&canvas, *ui);

    // the panel, the rule, and one icon per button - and not one label
    BOOST_CHECK(written.empty());
    BOOST_CHECK_EQUAL(canvas.batches().size(), 2U);

    const boost::shared_ptr<v3d::ui::component::Toolbar> bar =
        boost::dynamic_pointer_cast<v3d::ui::component::Toolbar>(ui->container("editor")->get("tools"));
    BOOST_REQUIRE(bar);

    // the column is as wide as the icon, not as the label it would otherwise draw
    const v3d::ui::paint::Dressing& dressing = drawing.dressing();
    BOOST_CHECK_CLOSE(bar->bound().size().x, dressing.iconSize + dressing.padding, 0.001F);
    BOOST_CHECK_CLOSE(drawing.insets(*ui).x, dressing.iconSize + dressing.padding + 1.0F, 0.001F);

    // and the icon is centred in the button's own box
    const v3d::type::geometry::Bound2D box = bar->button(0)->bound();
    BOOST_CHECK_CLOSE(canvas.vertices()[8].position.x,
        box.position().x + ((box.size().x - dressing.iconSize) * 0.5F), 0.001F);
}

/**
 * A button whose icon was never resolved falls back to its label, so a missing image does
 * not leave an unreadable strip.
 **/
BOOST_AUTO_TEST_CASE(an_unresolved_icon_leaves_the_label_drawn) {
    std::vector<Written> written;
    const v3d::ui::paint::ComponentRenderer drawing(
        [](std::string_view text) { return static_cast<float>(text.size()) * 10.0F; },
        [&written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            written.push_back(Written{ std::string(text), pen, colour });
        });

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::make_shared<v3d::ui::component::Button>();
    button->label("Select");
    button->icon("icons/select.png");
    button->size(glm::vec2(40.0F, 24.0F));
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
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "label", "name": "score", "label": "Score", "position": [5, 5] },
            { "type": "icon", "name": "logo", "source": "skins/logo.tga", "position": [5, 40], "visible": false }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);
    ui->resolveImages([](const std::string&) { return v3d::render::realtime::TextureHandle(2); });

    const v3d::ui::paint::ComponentRenderer drawing = renderer();
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));
    drawing.draw(&canvas, *ui);

    // the label is text, which is the app's to draw, and the hidden icon is nothing
    BOOST_CHECK(canvas.empty());

    const boost::shared_ptr<v3d::ui::component::Label> label =
        boost::dynamic_pointer_cast<v3d::ui::component::Label>(ui->container("hud")->get("score"));
    BOOST_REQUIRE(label);
    // it was drawn, so it holds the bounds it was drawn in
    BOOST_CHECK_CLOSE(label->bound().size().x, 50.0F, 0.001F);

    ui->container("hud")->get("logo")->visible(true);
    drawing.draw(&canvas, *ui);
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 4U);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
};  // namespace v3d::ui::documents
