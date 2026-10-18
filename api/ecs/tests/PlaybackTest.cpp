/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ecs/Previous.h>
#include <api/ecs/component/Playback.h>

#include <cstdint>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>

using v3d::ecs::component::Playback;

static_assert(v3d::ecs::Interpolable<Playback>);

namespace {

const uint32_t walk = 0;
const uint32_t run = 1;

/**
 * A playback a second into a two second walk, which loops.
 **/
Playback walking() {
    Playback playback;
    v3d::ecs::component::play(&playback, {.clip = walk, .duration = 2.0F, .loops = true, .fade = 0.0F});
    v3d::ecs::component::advance(&playback, 1.0F);
    return playback;
}

BOOST_AUTO_TEST_SUITE(playback_test)

/**
 * Nothing plays until a clip is asked for, and a step moves nothing.
 **/
BOOST_AUTO_TEST_CASE(playback_starts_with_nothing_test) {
    Playback playback;
    BOOST_CHECK_EQUAL(playback.clip, Playback::none);
    v3d::ecs::component::advance(&playback, 0.5F);
    BOOST_CHECK_EQUAL(playback.time, 0.0F);
}

/**
 * A step moves the clip's time by the step, and a looping clip's time is kept unwrapped.
 **/
BOOST_AUTO_TEST_CASE(playback_advances_by_the_step_test) {
    Playback playback = walking();
    BOOST_CHECK_EQUAL(playback.time, 1.0F);
    v3d::ecs::component::advance(&playback, 1.5F);
    BOOST_CHECK_EQUAL(playback.time, 2.5F);
}

/**
 * A clip that does not loop stops at its end.
 **/
BOOST_AUTO_TEST_CASE(playback_a_clamped_clip_stops_test) {
    Playback playback;
    v3d::ecs::component::play(&playback, {.clip = run, .duration = 1.0F, .loops = false, .fade = 0.0F});
    v3d::ecs::component::advance(&playback, 0.75F);
    v3d::ecs::component::advance(&playback, 0.75F);
    BOOST_CHECK_EQUAL(playback.time, 1.0F);
}

/**
 * Asking for the clip already playing changes nothing, so a game can ask every step.
 **/
BOOST_AUTO_TEST_CASE(playback_asking_again_changes_nothing_test) {
    Playback playback = walking();
    v3d::ecs::component::play(&playback, {.clip = walk, .duration = 2.0F, .loops = true, .fade = 0.25F});
    BOOST_CHECK_EQUAL(playback.time, 1.0F);
    BOOST_CHECK_EQUAL(playback.from, Playback::none);
}

/**
 * A fade's weight runs from 0 to 1 over its duration, and the clip faded from keeps playing
 * meanwhile. When the fade ends, from returns to none.
 **/
BOOST_AUTO_TEST_CASE(playback_a_fade_runs_its_length_test) {
    Playback playback = walking();
    v3d::ecs::component::play(&playback, {.clip = run, .duration = 1.0F, .loops = true, .fade = 0.5F});
    BOOST_CHECK_EQUAL(playback.clip, run);
    BOOST_CHECK_EQUAL(playback.time, 0.0F);
    BOOST_CHECK_EQUAL(playback.from, walk);
    BOOST_CHECK_EQUAL(playback.fade, 0.0F);

    v3d::ecs::component::advance(&playback, 0.25F);
    BOOST_CHECK_EQUAL(playback.fade, 0.5F);
    BOOST_CHECK_EQUAL(playback.fromTime, 1.25F);
    BOOST_CHECK_EQUAL(playback.from, walk);

    v3d::ecs::component::advance(&playback, 0.25F);
    BOOST_CHECK_EQUAL(playback.fade, 1.0F);
    BOOST_CHECK_EQUAL(playback.from, Playback::none);
}

/**
 * Playing no clip with a fade fades the old clip out into the rest pose. The fade reaches 1,
 * and from returns to none.
 **/
BOOST_AUTO_TEST_CASE(playback_fades_out_into_no_clip_test) {
    Playback playback = walking();
    v3d::ecs::component::play(&playback, {.clip = Playback::none, .duration = 0.0F, .loops = false, .fade = 0.5F});
    BOOST_CHECK_EQUAL(playback.clip, Playback::none);
    BOOST_CHECK_EQUAL(playback.from, walk);

    v3d::ecs::component::advance(&playback, 0.25F);
    BOOST_CHECK_EQUAL(playback.fade, 0.5F);
    BOOST_CHECK_EQUAL(playback.fromTime, 1.25F);

    v3d::ecs::component::advance(&playback, 0.25F);
    BOOST_CHECK_EQUAL(playback.fade, 1.0F);
    BOOST_CHECK_EQUAL(playback.from, Playback::none);
    BOOST_CHECK_EQUAL(playback.time, 0.0F);
}

/**
 * With nothing to fade from, or no fade asked for, a clip cuts in.
 **/
BOOST_AUTO_TEST_CASE(playback_cuts_with_nothing_to_fade_from_test) {
    Playback playback;
    v3d::ecs::component::play(&playback, {.clip = walk, .duration = 2.0F, .loops = true, .fade = 0.5F});
    BOOST_CHECK_EQUAL(playback.from, Playback::none);
    BOOST_CHECK_EQUAL(playback.fade, 1.0F);

    v3d::ecs::component::play(&playback, {.clip = run, .duration = 1.0F, .loops = true, .fade = 0.0F});
    BOOST_CHECK_EQUAL(playback.from, Playback::none);
}

/**
 * Between two steps either side of a loop's end the time runs forwards through the end, not
 * back through the clip, because the time is kept unwrapped.
 **/
BOOST_AUTO_TEST_CASE(playback_interpolating_across_a_wrap_runs_forwards_test) {
    Playback before = walking();
    v3d::ecs::component::advance(&before, 0.75F);
    Playback after = before;
    v3d::ecs::component::advance(&after, 0.5F);

    BOOST_CHECK_EQUAL(v3d::ecs::component::interpolate(before, after, 0.5F).time, 2.0F);
}

/**
 * Interpolating across a step that changed clip gives the later step.
 **/
BOOST_AUTO_TEST_CASE(playback_interpolating_across_a_change_of_clip_test) {
    const Playback before = walking();
    Playback after = before;
    v3d::ecs::component::play(&after, {.clip = run, .duration = 1.0F, .loops = true, .fade = 0.5F});
    v3d::ecs::component::advance(&after, 0.25F);

    const Playback drawn = v3d::ecs::component::interpolate(before, after, 0.5F);
    BOOST_CHECK_EQUAL(drawn.clip, run);
    BOOST_CHECK_EQUAL(drawn.time, after.time);
    BOOST_CHECK_EQUAL(drawn.fade, after.fade);
}

/**
 * Midway through a fade the weight and both times are drawn between the steps.
 **/
BOOST_AUTO_TEST_CASE(playback_interpolating_a_fade_test) {
    Playback before = walking();
    v3d::ecs::component::play(&before, {.clip = run, .duration = 1.0F, .loops = true, .fade = 1.0F});
    v3d::ecs::component::advance(&before, 0.25F);
    Playback after = before;
    v3d::ecs::component::advance(&after, 0.25F);

    const Playback drawn = v3d::ecs::component::interpolate(before, after, 0.5F);
    BOOST_CHECK_EQUAL(drawn.fade, 0.375F);
    BOOST_CHECK_EQUAL(drawn.time, 0.375F);
    BOOST_CHECK_EQUAL(drawn.fromTime, 1.375F);
}

/**
 * A looping clip's time is wrapped once it passes 4096 seconds, a little over an hour. Across
 * the step that wraps it, the time is still drawn forwards and still passes its markers.
 **/
BOOST_AUTO_TEST_CASE(playback_a_long_loop_is_rebased_test) {
    Playback before;
    v3d::ecs::component::play(&before, {.clip = walk, .duration = 2.0F, .loops = true, .fade = 0.0F});
    before.time = 4095.5F;
    Playback after = before;
    v3d::ecs::component::advance(&after, 1.0F);

    BOOST_CHECK_EQUAL(after.time, 0.5F);
    BOOST_CHECK_EQUAL(v3d::ecs::component::interpolate(before, after, 0.5F).time, 0.0F);
    // the end of the loop, at 4096, was passed by the step
    BOOST_CHECK_EQUAL(v3d::ecs::component::crossed(before, after, 2.0F), 1U);
}

/**
 * A marker is passed by the step that reaches it, and a step that changed clip passes none.
 **/
BOOST_AUTO_TEST_CASE(playback_markers_test) {
    const Playback before = walking();
    Playback after = before;
    v3d::ecs::component::advance(&after, 0.5F);
    BOOST_CHECK_EQUAL(v3d::ecs::component::crossed(before, after, 1.25F), 1U);
    BOOST_CHECK_EQUAL(v3d::ecs::component::crossed(before, after, 0.25F), 0U);

    Playback changed = before;
    v3d::ecs::component::play(&changed, {.clip = run, .duration = 1.0F, .loops = true, .fade = 0.0F});
    v3d::ecs::component::advance(&changed, 0.5F);
    BOOST_CHECK_EQUAL(v3d::ecs::component::crossed(before, changed, 0.25F), 0U);
}

/**
 * advance() moves every entity's playback, and snapshot() and interpolated() draw it between
 * steps as they do a Transform.
 **/
BOOST_AUTO_TEST_CASE(playback_on_the_registry_test) {
    entt::registry registry;
    const entt::entity entity = registry.create();
    registry.emplace<Playback>(entity, walking());

    v3d::ecs::snapshot<Playback>(registry);
    v3d::ecs::component::advance(registry, 0.5F);

    BOOST_CHECK_EQUAL(registry.get<Playback>(entity).time, 1.5F);
    BOOST_CHECK_EQUAL(v3d::ecs::interpolated<Playback>(registry, entity, 0.5F).time, 1.25F);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
