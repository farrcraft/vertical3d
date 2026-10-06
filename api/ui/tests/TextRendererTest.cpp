/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Canvas.h>
#include <api/ui/paint/TextRenderer.h>

#include <string>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

/**
 * A renderer over the shared font that packs the given glyphs. The upload hands back a
 * handle without a device, since nothing here draws to one.
 **/
boost::shared_ptr<v3d::ui::paint::TextRenderer> renderer(const wchar_t* charcodes) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>("data", logger);
    const v3d::ui::paint::TextRenderer::Upload upload = [](const boost::shared_ptr<v3d::image::Image>&) {
        return v3d::render::realtime::TextureHandle(1);
    };
    return boost::make_shared<v3d::ui::paint::TextRenderer>(assets, logger, upload,
        v3d::ui::paint::TextRenderer::baseSize, v3d::ui::paint::TextRenderer::defaultFont, charcodes);
}

// e with an acute accent, U+00E9, as UTF-8
const char* const eAcute = "\xC3\xA9";

};  // namespace

BOOST_AUTO_TEST_SUITE(text_renderer_test)

/**
 * UTF-8 decodes to one code point per character, at each sequence length.
 **/
BOOST_AUTO_TEST_CASE(utf8_decodes_one_code_point_per_character) {
    const std::u32string decoded = v3d::ui::paint::TextRenderer::decode("a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80");
    BOOST_REQUIRE_EQUAL(decoded.size(), 4u);
    BOOST_CHECK(decoded[0] == U'a');
    BOOST_CHECK(decoded[1] == static_cast<char32_t>(0xE9));
    BOOST_CHECK(decoded[2] == static_cast<char32_t>(0x20AC));
    BOOST_CHECK(decoded[3] == static_cast<char32_t>(0x1F600));
}

/**
 * Malformed UTF-8 becomes U+FFFD a byte at a time, and decoding carries on after it.
 **/
BOOST_AUTO_TEST_CASE(malformed_utf8_decodes_as_replacement_characters) {
    const char32_t replacement = 0xFFFD;

    // a lead byte with its continuation cut off
    const std::u32string truncated = v3d::ui::paint::TextRenderer::decode("\xC3");
    BOOST_REQUIRE_EQUAL(truncated.size(), 1u);
    BOOST_CHECK(truncated[0] == replacement);

    // an overlong encoding of '/', then a character that is well formed
    const std::u32string overlong = v3d::ui::paint::TextRenderer::decode("\xC0\xAF" "b");
    BOOST_REQUIRE_EQUAL(overlong.size(), 3u);
    BOOST_CHECK(overlong[0] == replacement);
    BOOST_CHECK(overlong[1] == replacement);
    BOOST_CHECK(overlong[2] == U'b');

    // an encoded surrogate
    const std::u32string surrogate = v3d::ui::paint::TextRenderer::decode("\xED\xA0\x80");
    BOOST_REQUIRE_EQUAL(surrogate.size(), 3u);
    BOOST_CHECK(surrogate[0] == replacement);
}

/**
 * A character outside the packed set measures as nothing and draws nothing, and asking for
 * it does not change what later measures return.
 **/
BOOST_AUTO_TEST_CASE(a_character_that_is_not_packed_measures_and_draws_nothing) {
    const boost::shared_ptr<v3d::ui::paint::TextRenderer> text = renderer(v3d::ui::paint::TextRenderer::ascii);
    BOOST_REQUIRE(text->loaded());

    const float a = text->width("a");
    BOOST_REQUIRE(a > 0.0f);

    BOOST_CHECK_EQUAL(text->width(eAcute), 0.0f);
    BOOST_CHECK_EQUAL(text->width(eAcute), 0.0f);
    BOOST_CHECK_CLOSE(text->width(std::string("a") + eAcute), a, 0.01f);

    v3d::render::realtime::Canvas canvas;
    canvas.resize(640, 480);
    text->draw(&canvas, eAcute, glm::vec2(10.0f, 100.0f), glm::vec4(1.0f));
    BOOST_CHECK(canvas.empty());

    // one quad for the 'a', and nothing for the character after it
    text->draw(&canvas, std::string("a") + eAcute, glm::vec2(10.0f, 100.0f), glm::vec4(1.0f));
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 4u);
}

/**
 * A packed character written as several UTF-8 bytes measures and draws as one glyph.
 **/
BOOST_AUTO_TEST_CASE(a_multibyte_character_is_one_glyph) {
    const boost::shared_ptr<v3d::ui::paint::TextRenderer> text = renderer(L"a\u00e9");
    BOOST_REQUIRE(text->loaded());

    const float one = text->width(eAcute);
    BOOST_REQUIRE(one > 0.0f);
    BOOST_CHECK_CLOSE(text->width(std::string(eAcute) + eAcute), one * 2.0f, 0.01f);

    v3d::render::realtime::Canvas canvas;
    canvas.resize(640, 480);
    text->draw(&canvas, eAcute, glm::vec2(10.0f, 100.0f), glm::vec4(1.0f));
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 4u);
}

BOOST_AUTO_TEST_SUITE_END()
