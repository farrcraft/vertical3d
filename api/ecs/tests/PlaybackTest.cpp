/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ecs/Previous.h>
#include <api/ecs/component/Playback.h>

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
    v3d::ecs::component::play(&playback, walk, 2.0f, true, 0.0f);
    v3d::ecs::component::advance(&playback, 1.0f);
    return playback;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(playback_test)

/**
 * Nothing plays until a clip is asked for, and a step moves nothing.
 **/
BOOST_AUTO_TEST_CASE(playback_starts_with_nothing_test) {
    Playback playback;
    BOOST_CHECK_EQUAL(playback.clip, Playback::none);
    v3d::ecs::component::advance(&playback, 0.5f);
    BOOST_CHECK_EQUAL(playback.time, 0.0f);
}

/**
 * A step moves the clip's time by the step, and a looping clip's time is kept unwrapped.
 **/
BOOST_AUTO_TEST_CASE(playback_advances_by_the_step_test) {
    Playback playback = walking();
    BOOST_CHECK_EQUAL(playback.time, 1.0f);
    v3d::ecs::component::advance(&playback, 1.5f);
    BOOST_CHECK_EQUAL(playback.time, 2.5f);
}

/**
 * A clip that does not loop stops at its end.
 **/
BOOST_AUTO_TEST_CASE(playback_a_clamped_clip_stops_test) {
    Playback playback;
    v3d::ecs::component::play(&playback, run, 1.0f, false, 0.0f);
    v3d::ecs::component::advance(&playback, 0.75f);
    v3d::ecs::component::advance(&playback, 0.75f);
    BOOST_CHECK_EQUAL(playback.time, 1.0f);
}

/**
 * Asking for the clip already playing changes nothing, so a game can ask every step.
 **/
BOOST_AUTO_TEST_CASE(playback_asking_again_changes_nothing_test) {
    Playback playback = walking();
    v3d::ecs::component::play(&playback, walk, 2.0f, true, 0.25f);
    BOOST_CHECK_EQUAL(playback.time, 1.0f);
    BOOST_CHECK_EQUAL(playback.from, Playback::none);
}

/**
 * A fade's weight runs from 0 to 1 over its duration, the clip faded from keeps playing
 * meanwhile, and once the fade is done that clip is let go.
 **/
BOOST_AUTO_TEST_CASE(playback_a_fade_runs_its_length_test) {
    Playback playback = walking();
    v3d::ecs::component::play(&playback, run, 1.0f, true, 0.5f);
    BOOST_CHECK_EQUAL(playback.clip, run);
    BOOST_CHECK_EQUAL(playback.time, 0.0f);
    BOOST_CHECK_EQUAL(playback.from, walk);
    BOOST_CHECK_EQUAL(playback.fade, 0.0f);

    v3d::ecs::component::advance(&playback, 0.25f);
    BOOST_CHECK_EQUAL(playback.fade, 0.5f);
    BOOST_CHECK_EQUAL(playback.fromTime, 1.25f);
    BOOST_CHECK_EQUAL(playback.from, walk);

    v3d::ecs::component::advance(&playback, 0.25f);
    BOOST_CHECK_EQUAL(playback.fade, 1.0f);
    BOOST_CHECK_EQUAL(playback.from, Playback::none);
}

/**
 * With nothing to fade from, or no fade asked for, a clip cuts in.
 **/
BOOST_AUTO_TEST_CASE(playback_cuts_with_nothing_to_fade_from_test) {
    Playback playback;
    v3d::ecs::component::play(&playback, walk, 2.0f, true, 0.5f);
    BOOST_CHECK_EQUAL(playback.from, Playback::none);
    BOOST_CHECK_EQUAL(playback.fade, 1.0f);

    v3d::ecs::component::play(&playback, run, 1.0f, true, 0.0f);
    BOOST_CHECK_EQUAL(playback.from, Playback::none);
}

/**
 * Between two steps either side of a loop's end the time runs forwards through the end, not
 * back through the clip, because the time is kept unwrapped.
 **/
BOOST_AUTO_TEST_CASE(playback_interpolating_across_a_wrap_runs_forwards_test) {
    Playback before = walking();
    v3d::ecs::component::advance(&before, 0.75f);
    Playback after = before;
    v3d::ecs::component::advance(&after, 0.5f);

    BOOST_CHECK_EQUAL(v3d::ecs::component::interpolate(before, after, 0.5f).time, 2.0f);
}

/**
 * Across a step that changed clip there is no halfway, so the later step is drawn.
 **/
BOOST_AUTO_TEST_CASE(playback_interpolating_across_a_change_of_clip_test) {
    const Playback before = walking();
    Playback after = before;
    v3d::ecs::component::play(&after, run, 1.0f, true, 0.5f);
    v3d::ecs::component::advance(&after, 0.25f);

    const Playback drawn = v3d::ecs::component::interpolate(before, after, 0.5f);
    BOOST_CHECK_EQUAL(drawn.clip, run);
    BOOST_CHECK_EQUAL(drawn.time, after.time);
    BOOST_CHECK_EQUAL(drawn.fade, after.fade);
}

/**
 * Midway through a fade the weight and both times are drawn between the steps.
 **/
BOOST_AUTO_TEST_CASE(playback_interpolating_a_fade_test) {
    Playback before = walking();
    v3d::ecs::component::play(&before, run, 1.0f, true, 1.0f);
    v3d::ecs::component::advance(&before, 0.25f);
    Playback after = before;
    v3d::ecs::component::advance(&after, 0.25f);

    const Playback drawn = v3d::ecs::component::interpolate(before, after, 0.5f);
    BOOST_CHECK_EQUAL(drawn.fade, 0.375f);
    BOOST_CHECK_EQUAL(drawn.time, 0.375f);
    BOOST_CHECK_EQUAL(drawn.fromTime, 1.375f);
}

/**
 * A looping clip that has played for over an hour has whole loops taken off its time, and is
 * still drawn forwards and still passes its markers across the step that did it.
 **/
BOOST_AUTO_TEST_CASE(playback_a_long_loop_is_rebased_test) {
    Playback before;
    v3d::ecs::component::play(&before, walk, 2.0f, true, 0.0f);
    before.time = 4095.5f;
    Playback after = before;
    v3d::ecs::component::advance(&after, 1.0f);

    BOOST_CHECK_EQUAL(after.time, 0.5f);
    BOOST_CHECK_EQUAL(v3d::ecs::component::interpolate(before, after, 0.5f).time, 0.0f);
    // the end of the loop, at 4096, was passed by the step
    BOOST_CHECK_EQUAL(v3d::ecs::component::crossed(before, after, 2.0f), 1u);
}

/**
 * A marker is passed by the step that reaches it, and a step that changed clip passes none.
 **/
BOOST_AUTO_TEST_CASE(playback_markers_test) {
    const Playback before = walking();
    Playback after = before;
    v3d::ecs::component::advance(&after, 0.5f);
    BOOST_CHECK_EQUAL(v3d::ecs::component::crossed(before, after, 1.25f), 1u);
    BOOST_CHECK_EQUAL(v3d::ecs::component::crossed(before, after, 0.25f), 0u);

    Playback changed = before;
    v3d::ecs::component::play(&changed, run, 1.0f, true, 0.0f);
    v3d::ecs::component::advance(&changed, 0.5f);
    BOOST_CHECK_EQUAL(v3d::ecs::component::crossed(before, changed, 0.25f), 0u);
}

/**
 * Every entity's playback is advanced, and snapshot and interpolated draw it between steps as
 * they do a Transform.
 **/
BOOST_AUTO_TEST_CASE(playback_on_the_registry_test) {
    entt::registry registry;
    const entt::entity entity = registry.create();
    registry.emplace<Playback>(entity, walking());

    v3d::ecs::snapshot<Playback>(registry);
    v3d::ecs::component::advance(registry, 0.5f);

    BOOST_CHECK_EQUAL(registry.get<Playback>(entity).time, 1.5f);
    BOOST_CHECK_EQUAL(v3d::ecs::interpolated<Playback>(registry, entity, 0.5f).time, 1.25f);
}

BOOST_AUTO_TEST_SUITE_END()
