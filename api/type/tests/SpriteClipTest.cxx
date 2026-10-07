/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/animation/SpriteClip.h>

#include <stdexcept>
#include <vector>

#include <boost/test/unit_test.hpp>

using v3d::type::animation::SpriteClip;

namespace {

/**
 * Three frames of a strip four regions wide, shown for a quarter, a half and a quarter of a
 * second - lengths a float sums exactly, so every boundary is where the test says it is.
 **/
std::vector<SpriteClip::Frame> strip() {
    return {
        SpriteClip::Frame{glm::vec2(0.0f, 0.0f), glm::vec2(0.25f, 1.0f), 0.25f},
        SpriteClip::Frame{glm::vec2(0.25f, 0.0f), glm::vec2(0.5f, 1.0f), 0.5f},
        SpriteClip::Frame{glm::vec2(0.5f, 0.0f), glm::vec2(0.75f, 1.0f), 0.25f}
    };
}

/**
 * @return which of the strip's frames a clip shows at a time, by the region's left edge
 **/
int showing(const SpriteClip& clip, float time) {
    return static_cast<int>(clip.frame(time).uv0.x * 4.0f);
}

};  // namespace

BOOST_AUTO_TEST_SUITE(sprite_clip_test)

/**
 * Each frame is shown from its start up to the next frame's start, and the clock runs for the
 * frames' summed length.
 **/
BOOST_AUTO_TEST_CASE(sprite_clip_each_frame_across_its_span_test) {
    const SpriteClip clip(strip(), true);

    BOOST_CHECK_EQUAL(clip.clock().duration(), 1.0f);
    BOOST_CHECK_EQUAL(showing(clip, 0.0f), 0);
    BOOST_CHECK_EQUAL(showing(clip, 0.2f), 0);
    BOOST_CHECK_EQUAL(showing(clip, 0.25f), 1);
    BOOST_CHECK_EQUAL(showing(clip, 0.7f), 1);
    BOOST_CHECK_EQUAL(showing(clip, 0.75f), 2);
    BOOST_CHECK_EQUAL(showing(clip, 0.99f), 2);
    BOOST_CHECK(clip.frame(0.5f).uv1 == glm::vec2(0.5f, 1.0f));
}

/**
 * A looping clip starts again at its first frame, from a time the clock keeps unwrapped.
 **/
BOOST_AUTO_TEST_CASE(sprite_clip_a_looping_clip_wraps_test) {
    const SpriteClip clip(strip(), true);

    BOOST_CHECK_EQUAL(showing(clip, 1.0f), 0);
    BOOST_CHECK_EQUAL(showing(clip, 1.3f), 1);
    BOOST_CHECK_EQUAL(showing(clip, clip.clock().advance(0.9f, 0.2f)), 0);
}

/**
 * A clamped clip holds its last frame at its end and after it.
 **/
BOOST_AUTO_TEST_CASE(sprite_clip_a_clamped_clip_holds_its_last_frame_test) {
    const SpriteClip clip(strip(), false);

    const float end = clip.clock().advance(0.9f, 0.5f);
    BOOST_CHECK(clip.clock().finished(end));
    BOOST_CHECK_EQUAL(showing(clip, end), 2);
    BOOST_CHECK_EQUAL(showing(clip, 5.0f), 2);
}

/**
 * A clip with nothing to show, or with a frame that is never shown, is refused.
 **/
BOOST_AUTO_TEST_CASE(sprite_clip_refuses_what_it_cannot_show_test) {
    BOOST_CHECK_THROW(SpriteClip(std::vector<SpriteClip::Frame>(), true), std::invalid_argument);

    std::vector<SpriteClip::Frame> frames = strip();
    frames[1].duration = 0.0f;
    BOOST_CHECK_THROW(SpriteClip(frames, true), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
