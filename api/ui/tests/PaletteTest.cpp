/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/Palette.h>

#include <array>
#include <cmath>
#include <cstddef>

#include <boost/test/unit_test.hpp>

#include <glm/vec4.hpp>

namespace {

using v3d::ui::paint::Dressing;
using v3d::ui::paint::Palette;
using v3d::ui::paint::contrastRatio;

/**
 * A colour from its three bytes, as a hex code writes them.
 **/
glm::vec4 hex(unsigned int code) {
    return {static_cast<float>((code >> 16U) & 0xFFU) / 255.0F, static_cast<float>((code >> 8U) & 0xFFU) / 255.0F,
        static_cast<float>(code & 0xFFU) / 255.0F, 1.0F};
}

/**
 * Whether two colours are within a given number of byte steps of each other in every channel.
 **/
bool near(const glm::vec4& one, const glm::vec4& other, float bytes) {
    const float tolerance = bytes / 255.0F;
    return std::abs(one.r - other.r) <= tolerance && std::abs(one.g - other.g) <= tolerance &&
        std::abs(one.b - other.b) <= tolerance;
}

/**
 * Every text role is checked against every surface it is drawn on.
 **/
void checkLegible(const Dressing& dressing) {
    for (const glm::vec4& surface : {dressing.panel, dressing.titleBar, dressing.widget, dressing.hover, dressing.bar}) {
        BOOST_CHECK_GE(contrastRatio(dressing.text, surface), 4.5F);
        BOOST_CHECK_GE(contrastRatio(dressing.secondaryText, surface), 4.5F);
    }
    for (const glm::vec4& surface : {dressing.highlight, dressing.hover, dressing.pressed}) {
        BOOST_CHECK_GE(contrastRatio(dressing.activeText, surface), 4.5F);
    }
    for (const glm::vec4& surface : {dressing.panel, dressing.widget}) {
        BOOST_CHECK_GE(contrastRatio(dressing.disabledText, surface), 3.0F);
        BOOST_CHECK_GE(contrastRatio(dressing.placeholder, surface), 3.0F);
    }
    for (const glm::vec4& edge : {dressing.border, dressing.focus, dressing.mark, dressing.caret}) {
        BOOST_CHECK_GE(contrastRatio(edge, dressing.panel), 3.0F);
    }
}

BOOST_AUTO_TEST_SUITE(palette_test)

/**
 * Black against white is the widest contrast WCAG measures, and a colour against itself the
 * narrowest. The order of the two does not matter.
 **/
BOOST_AUTO_TEST_CASE(contrast_runs_from_one_to_twenty_one) {
    const glm::vec4 black(0.0F, 0.0F, 0.0F, 1.0F);
    const glm::vec4 white(1.0F, 1.0F, 1.0F, 1.0F);
    BOOST_CHECK_CLOSE(contrastRatio(black, white), 21.0F, 0.01F);
    BOOST_CHECK_CLOSE(contrastRatio(white, black), 21.0F, 0.01F);
    BOOST_CHECK_CLOSE(contrastRatio(white, white), 1.0F, 0.01F);
}

/**
 * The built-in palette is the design's scale: the base is the third neutral step and the
 * accent the ninth accent step, and every neutral step lands on the colour it was measured from.
 * Secondary text is a step lighter than the design drew it, so that it reads on a hover.
 **/
BOOST_AUTO_TEST_CASE(the_built_in_palette_is_the_design_scale) {
    const Palette palette;
    BOOST_CHECK(palette.dark());
    BOOST_CHECK(near(palette.neutral()[2], Palette::base, 1.0F));
    BOOST_CHECK(near(palette.accented()[8], Palette::accent, 1.0F));

    const std::array<unsigned int, Palette::steps> design{
        0x111214U, 0x17181bU, 0x1d1f23U, 0x25282dU, 0x2a2d32U, 0x2e3137U,
        0x383b42U, 0x454950U, 0x6b7079U, 0x858b94U, 0xa3a7b1U, 0xe4e6eaU};
    for (std::size_t step = 0; step < Palette::steps; step++) {
        BOOST_TEST_CONTEXT("step " << step) {
            BOOST_CHECK(near(palette.neutral()[step], hex(design[step]), 2.0F));
        }
    }
}

/**
 * The default dressings of both ui models meet the contrast each role needs.
 **/
BOOST_AUTO_TEST_CASE(the_built_in_roles_are_legible) {
    checkLegible(Dressing());
    checkLegible(Dressing::tools());
}

/**
 * The immediate layer's tracks sit at its widgets' level, and the retained ui's sit below them.
 **/
BOOST_AUTO_TEST_CASE(the_two_models_place_tracks_differently) {
    const Dressing chrome;
    const Dressing tools = Dressing::tools();
    BOOST_CHECK(tools.track == tools.widget);
    BOOST_CHECK(chrome.track != chrome.widget);
    BOOST_CHECK(chrome.panel == tools.panel);
}

/**
 * A set of tabs is a recess below the panel, and the chosen tab is raised out of it to a
 * widget's level, in both models. The chosen tab is not the highlight a checked toggle is
 * filled with, so a tab reads as the place shown and not as a setting that is on.
 **/
BOOST_AUTO_TEST_CASE(a_chosen_tab_is_raised_and_not_highlighted) {
    for (const Dressing& dressing : {Dressing(), Dressing::tools()}) {
        BOOST_CHECK(dressing.tab == dressing.bar);
        BOOST_CHECK(dressing.chosen == dressing.widget);
        BOOST_CHECK(dressing.chosen != dressing.highlight);
    }
}

/**
 * Contrast spreads the steps apart above 1 and draws them together below it. At 0 every
 * neutral step has the base's lightness, and differs from it only in chroma.
 **/
BOOST_AUTO_TEST_CASE(contrast_spreads_the_steps) {
    const Palette plain(Palette::base, Palette::accent, 1.0F);
    const Palette strong(Palette::base, Palette::accent, 1.25F);
    const Palette flat(Palette::base, Palette::accent, 0.0F);

    const float plainRatio = contrastRatio(plain.neutral()[11], plain.neutral()[2]);
    BOOST_CHECK_GT(contrastRatio(strong.neutral()[11], strong.neutral()[2]), plainRatio);
    for (const glm::vec4& step : flat.neutral()) {
        BOOST_CHECK_LT(contrastRatio(step, Palette::base), 1.05F);
    }
}

/**
 * A base lighter than mid grey is a light theme: text is darker than the surface under it,
 * and every role still meets its contrast.
 **/
BOOST_AUTO_TEST_CASE(a_light_base_puts_dark_text_on_light_surfaces) {
    const Palette light(hex(0xf4f5f7U), Palette::accent, 1.0F);
    BOOST_CHECK(!light.dark());

    Dressing dressing;
    light.dress(Palette::Use::Chrome, &dressing);
    BOOST_CHECK_LT(dressing.text.r, dressing.panel.r);
    BOOST_CHECK_LT(dressing.activeText.r, dressing.panel.r);
    BOOST_CHECK_GE(contrastRatio(dressing.text, dressing.panel), 4.5F);
    BOOST_CHECK_GE(contrastRatio(dressing.activeText, dressing.highlight), 4.5F);
}

/**
 * An accent too light to carry white text still gives a highlight that does: the highlight
 * darkens until the two meet 4.5:1.
 **/
BOOST_AUTO_TEST_CASE(a_light_accent_still_gives_a_legible_highlight) {
    const Palette yellow(Palette::base, hex(0xffd94dU), 1.0F);
    Dressing dressing;
    yellow.dress(Palette::Use::Chrome, &dressing);
    BOOST_CHECK_LT(contrastRatio(dressing.activeText, yellow.accented()[7]), 4.5F);
    BOOST_CHECK_GE(contrastRatio(dressing.activeText, dressing.highlight), 4.5F);
}

/**
 * The palette writes every colour role and leaves the metrics alone.
 **/
BOOST_AUTO_TEST_CASE(dressing_leaves_the_metrics_alone) {
    Dressing dressing;
    dressing.lineHeight = 99.0F;
    dressing.focus = glm::vec4(1.0F, 0.0F, 0.0F, 1.0F);

    Palette(hex(0x202830U), hex(0x40c080U), 1.0F).dress(Palette::Use::Chrome, &dressing);
    BOOST_CHECK_CLOSE(dressing.lineHeight, 99.0F, 0.001F);
    BOOST_CHECK(near(dressing.focus, hex(0x40c080U), 1.0F));
    BOOST_CHECK(near(dressing.panel, hex(0x202830U), 1.0F));
}

/**
 * A scaled dressing multiplies every size and keeps every colour. An outline is rounded down to
 * whole pixels, is never thinner than one, and stays none when it was none.
 **/
BOOST_AUTO_TEST_CASE(a_scaled_dressing_multiplies_its_sizes) {
    Dressing dressing;
    dressing.radius = 4.0F;
    const Dressing doubled = dressing.scaled(2.0F);
    BOOST_CHECK_CLOSE(doubled.padding, dressing.padding * 2.0F, 0.001F);
    BOOST_CHECK_CLOSE(doubled.lineHeight, dressing.lineHeight * 2.0F, 0.001F);
    BOOST_CHECK_CLOSE(doubled.radius, 8.0F, 0.001F);
    BOOST_CHECK_CLOSE(doubled.borderWidth, 2.0F, 0.001F);
    BOOST_CHECK_CLOSE(doubled.titleWeight, dressing.titleWeight, 0.001F);
    BOOST_CHECK(doubled.highlight == dressing.highlight);

    BOOST_CHECK_CLOSE(dressing.scaled(1.5F).borderWidth, 1.0F, 0.001F);
    BOOST_CHECK_CLOSE(dressing.scaled(0.5F).borderWidth, 1.0F, 0.001F);
    BOOST_CHECK_CLOSE(Dressing::outline(0.0F, 2.0F), 0.0F, 0.001F);
    BOOST_CHECK_CLOSE(Dressing::outline(2.0F, 1.5F), 3.0F, 0.001F);
}

/**
 * Resizing a dressing takes another's sizes and keeps its own colours.
 **/
BOOST_AUTO_TEST_CASE(resizing_a_dressing_keeps_its_colours) {
    Dressing dressing;
    dressing.text = glm::vec4(1.0F, 0.0F, 0.0F, 1.0F);
    const Dressing large = Dressing().scaled(3.0F);
    dressing.resize(large);
    BOOST_CHECK_CLOSE(dressing.padding, large.padding, 0.001F);
    BOOST_CHECK_CLOSE(dressing.borderWidth, large.borderWidth, 0.001F);
    BOOST_CHECK(dressing.text == glm::vec4(1.0F, 0.0F, 0.0F, 1.0F));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
