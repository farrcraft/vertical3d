/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/paint/TextRenderer.h>

#include <string>
#include <string_view>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

/**
 * A renderer over the shared font that packs the given glyphs. The upload hands back a
 * handle without a device, since nothing here draws to one.
 **/
boost::shared_ptr<v3d::ui::paint::TextRenderer> renderer(std::string_view charset) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>("data", logger);
    const v3d::ui::paint::TextRenderer::Upload upload = [](const boost::shared_ptr<v3d::image::Image>&) {
        return v3d::render::realtime::TextureHandle(1);
    };
    return boost::make_shared<v3d::ui::paint::TextRenderer>(assets, logger, upload,
        v3d::ui::paint::TextRenderer::baseSize, v3d::ui::paint::TextRenderer::defaultFont, charset);
}

// e with an acute accent, U+00E9, as UTF-8
const char* const eAcute = "\xC3\xA9";

BOOST_AUTO_TEST_SUITE(text_renderer_test)

/**
 * A character outside the packed set measures as nothing and draws nothing, and asking for
 * it does not change what later measures return.
 **/
BOOST_AUTO_TEST_CASE(a_character_that_is_not_packed_measures_and_draws_nothing) {
    const boost::shared_ptr<v3d::ui::paint::TextRenderer> text = renderer(v3d::ui::paint::TextRenderer::ascii);
    BOOST_REQUIRE(text->loaded());

    const float letterWidth = text->width("a");
    BOOST_REQUIRE(letterWidth > 0.0F);

    BOOST_CHECK_EQUAL(text->width(eAcute), 0.0F);
    BOOST_CHECK_EQUAL(text->width(eAcute), 0.0F);
    BOOST_CHECK_CLOSE(text->width(std::string("a") + eAcute), letterWidth, 0.01F);

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(640, 480));
    text->draw(&canvas, eAcute, glm::vec2(10.0F, 100.0F), glm::vec4(1.0F));
    BOOST_CHECK(canvas.empty());

    // one quad for the 'a', and nothing for the character after it
    text->draw(&canvas, std::string("a") + eAcute, glm::vec2(10.0F, 100.0F), glm::vec4(1.0F));
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 4U);
}

/**
 * A packed character written as several UTF-8 bytes measures and draws as one glyph.
 **/
BOOST_AUTO_TEST_CASE(a_multibyte_character_is_one_glyph) {
    const boost::shared_ptr<v3d::ui::paint::TextRenderer> text = renderer(std::string("a") + eAcute);
    BOOST_REQUIRE(text->loaded());

    const float one = text->width(eAcute);
    BOOST_REQUIRE(one > 0.0F);
    BOOST_CHECK_CLOSE(text->width(std::string(eAcute) + eAcute), one * 2.0F, 0.01F);

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(640, 480));
    text->draw(&canvas, eAcute, glm::vec2(10.0F, 100.0F), glm::vec4(1.0F));
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 4U);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
