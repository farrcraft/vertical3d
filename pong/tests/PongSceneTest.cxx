/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ecs/Previous.h>
#include <api/ecs/component/Position1D.h>
#include <api/ecs/component/Position2D.h>
#include <api/event/kind/Sound.h>
#include <pong/src/PongScene.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace {

/**
 * The scene reports what happened by triggering a sound, so the clips a tick fires are the
 * only record of which branch it took.
 **/
struct Sounds final {
    void heard(const v3d::event::kind::Sound& sound) {
        clips_.push_back(std::string(sound.clip()));
    }

    bool has(const std::string& clip) const {
        return std::ranges::any_of(clips_, [&clip](const std::string& played) { return played == clip; });
    }

    std::vector<std::string> clips_;
};

/**
 * The fixed step the engine simulates at, in seconds. Every scene speed below is per second.
 * It is repeated here rather than taken from v3d::engine::Accumulator, because this test
 * links neither the engine nor a device.
 **/
constexpr float STEP = 1.0f / 60.0f;

/**
 * Speeds are per second and a sixtieth is not exactly representable, so a position is
 * asserted to within a hundredth of a pixel rather than exactly.
 **/
bool near(const glm::vec2& lhs, const glm::vec2& rhs) {
    return std::abs(lhs.x - rhs.x) < 0.01f && std::abs(lhs.y - rhs.y) < 0.01f;
}

/**
 * A scene reset and listening. Everything below measures against the 800x600 court, the
 * units the collision tests are written in.
 **/
struct Fixture final {
    Fixture() :
        dispatcher_(boost::make_shared<entt::dispatcher>()),
        scene_(&registry_, dispatcher_) {
        dispatcher_->sink<v3d::event::kind::Sound>().connect<&Sounds::heard>(sounds_);
        scene_.reset();
    }

    entt::registry registry_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    Sounds sounds_;
    PongScene scene_;
};

};  // namespace

/**
 * reset() centres both paddles and the ball and serves to the left, the state a round
 * begins in.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_reset_test) {
    Fixture fixture;

    BOOST_TEST((fixture.scene_.ball().position() == glm::vec2(400.0f, 300.0f)));
    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(-60.0f, 0.0f)));
    BOOST_TEST(fixture.scene_.ball().size() == 10.0f);
    BOOST_TEST(fixture.scene_.left().position() == 300.0f);
    BOOST_TEST(fixture.scene_.right().position() == 300.0f);
    BOOST_TEST(fixture.scene_.left().score() == 0);
    BOOST_TEST(fixture.scene_.right().score() == 0);
}

/**
 * A tick with nothing in reach moves the ball by one step of its velocity and fires
 * nothing. Sixty pixels a second over a sixtieth of a second is one pixel.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_tick_moves_the_ball_test) {
    Fixture fixture;

    fixture.scene_.tick(STEP);

    BOOST_TEST(near(fixture.scene_.ball().position(), glm::vec2(399.0f, 300.0f)));
    BOOST_TEST(fixture.sounds_.clips_.empty());
}

/**
 * A paused scene does nothing at all. The game is paused while the menu is open.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paused_test) {
    Fixture fixture;
    fixture.scene_.state().pause(true);

    fixture.scene_.tick(STEP);

    BOOST_TEST((fixture.scene_.ball().position() == glm::vec2(400.0f, 300.0f)));
    BOOST_TEST(fixture.sounds_.clips_.empty());
}

/**
 * The paddle test is a box against a box, so the ball is met when it is within half its own
 * size of the paddle's face and within half the paddle's length of its centre.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_left_paddle_collision_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(20.0f, 300.0f));
    fixture.scene_.ball().direction(glm::vec2(-60.0f, 0.0f));

    fixture.scene_.tick(STEP);

    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(60.0f, 0.0f)));
    BOOST_TEST(fixture.sounds_.has("hit"));
}

BOOST_AUTO_TEST_CASE(pong_scene_right_paddle_collision_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(780.0f, 300.0f));
    fixture.scene_.ball().direction(glm::vec2(60.0f, 0.0f));

    fixture.scene_.tick(STEP);

    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(-60.0f, 0.0f)));
    BOOST_TEST(fixture.sounds_.has("hit"));
}

/**
 * A ball that has already passed the paddle's face, as a fast one does between two steps, is
 * still returned rather than let through to the edge behind the paddle.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paddle_past_the_face_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(8.0f, 300.0f));
    fixture.scene_.ball().direction(glm::vec2(-60.0f, 0.0f));

    fixture.scene_.tick(STEP);

    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(60.0f, 0.0f)));
    BOOST_TEST(fixture.sounds_.has("hit"));

    Fixture right;
    right.scene_.ball().position(glm::vec2(792.0f, 300.0f));
    right.scene_.ball().direction(glm::vec2(60.0f, 0.0f));

    right.scene_.tick(STEP);

    BOOST_TEST((right.scene_.ball().direction() == glm::vec2(-60.0f, 0.0f)));
    BOOST_TEST(right.sounds_.has("hit"));
}

/**
 * A ball level with the paddle's face but past the end of it is not met. This case
 * separates a save from a point.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paddle_miss_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(20.0f, 100.0f));
    fixture.scene_.ball().direction(glm::vec2(-60.0f, 0.0f));

    fixture.scene_.tick(STEP);

    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(-60.0f, 0.0f)));
    BOOST_TEST(!fixture.sounds_.has("hit"));
}

/**
 * A paddle travelling as it meets the ball carries the return the way it is going, on both
 * sides: a paddle moving up sends the ball up, which is towards smaller y.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paddle_travel_angles_the_return_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(20.0f, 300.0f));
    fixture.scene_.ball().direction(glm::vec2(-60.0f, 0.0f));
    fixture.scene_.left().up(true);

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.ball().direction().x == 60.0f);
    BOOST_TEST(fixture.scene_.ball().direction().y < 0.0f);

    Fixture right;
    right.scene_.ball().position(glm::vec2(780.0f, 300.0f));
    right.scene_.ball().direction(glm::vec2(60.0f, 0.0f));
    right.scene_.right().down(true);

    right.scene_.tick(STEP);

    BOOST_TEST(right.scene_.ball().direction().x == -60.0f);
    BOOST_TEST(right.scene_.ball().direction().y > 0.0f);
}

/**
 * Where the ball meets the paddle sets the angle it goes back at: flat off the centre,
 * downwards off the lower half and upwards off the upper, steepest off the ends. The speed
 * is kept, whatever the angle.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paddle_angles_by_where_it_is_struck_test) {
    const float struck[] = { 290.0f, 300.0f, 310.0f, 330.0f };
    float lastAngle = -1.0f;
    for (const float y : struck) {
        Fixture fixture;
        fixture.scene_.ball().position(glm::vec2(20.0f, y));
        fixture.scene_.ball().direction(glm::vec2(-60.0f, 0.0f));

        fixture.scene_.tick(STEP);

        const glm::vec2 returned = fixture.scene_.ball().direction();
        BOOST_TEST(returned.x > 0.0f);
        BOOST_TEST(glm::length(returned) == 60.0f, boost::test_tools::tolerance(0.001f));
        const float angle = std::atan2(returned.y, returned.x);
        BOOST_TEST(angle > lastAngle);
        lastAngle = angle;
    }
    // the last was struck by the very end, at the steepest return: fifty degrees, downwards
    BOOST_TEST(lastAngle == 0.8727f, boost::test_tools::tolerance(0.001f));

    // an angled ball struck by the centre goes back flat rather than retracing its line
    Fixture flat;
    flat.scene_.ball().position(glm::vec2(20.0f, 300.0f));
    flat.scene_.ball().direction(glm::vec2(-48.0f, 36.0f));

    flat.scene_.tick(STEP);

    BOOST_TEST(flat.scene_.ball().direction().x == 60.0f, boost::test_tools::tolerance(0.001f));
    BOOST_TEST(flat.scene_.ball().direction().y == 0.0f, boost::test_tools::tolerance(0.001f));
}

/**
 * A ball already heading away from a paddle is not returned again on the step after it was,
 * however deep past the face it still is.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paddle_returns_once_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(8.0f, 300.0f));
    fixture.scene_.ball().direction(glm::vec2(60.0f, 0.0f));

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.ball().direction().x == 60.0f);
    BOOST_TEST(!fixture.sounds_.has("hit"));
}

/**
 * The ball reaching an edge is a point to the far paddle, and the ball goes back to the
 * middle serving towards whoever conceded it.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_left_edge_scores_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(5.0f, 100.0f));
    fixture.scene_.ball().direction(glm::vec2(-60.0f, 0.0f));

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.right().score() == 1);
    BOOST_TEST(fixture.scene_.left().score() == 0);
    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(-60.0f, 0.0f)));
    BOOST_TEST(fixture.scene_.left().position() == 300.0f);
    BOOST_TEST(fixture.sounds_.has("score"));
}

/**
 * A point puts the ball back on the centre spot rather than moving it there, so the frame after
 * it is drawn from the centre and not swept across the court from the edge it left by.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_a_point_settles_the_ball_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(5.0f, 100.0f));
    fixture.scene_.ball().direction(glm::vec2(-60.0f, 0.0f));
    fixture.scene_.left().position(500.0f);

    v3d::ecs::snapshot<v3d::ecs::component::Position2D>(fixture.registry_);
    v3d::ecs::snapshot<v3d::ecs::component::Position1D>(fixture.registry_);
    fixture.scene_.tick(STEP);

    BOOST_TEST(near(fixture.scene_.ball().drawn(0.0f), glm::vec2(400.0f, 300.0f)));
    BOOST_TEST(fixture.scene_.left().drawn(0.0f) == 300.0f);
}

/**
 * Between two steps the ball is drawn between where the steps left it.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_ball_drawn_between_steps_test) {
    Fixture fixture;

    v3d::ecs::snapshot<v3d::ecs::component::Position2D>(fixture.registry_);
    fixture.scene_.tick(STEP);

    BOOST_TEST(near(fixture.scene_.ball().drawn(0.5f), glm::vec2(399.5f, 300.0f)));
}

BOOST_AUTO_TEST_CASE(pong_scene_right_edge_scores_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(795.0f, 100.0f));
    fixture.scene_.ball().direction(glm::vec2(60.0f, 0.0f));

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.left().score() == 1);
    BOOST_TEST(fixture.scene_.right().score() == 0);
    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(60.0f, 0.0f)));
    BOOST_TEST(fixture.sounds_.has("score"));
}

/**
 * Top and bottom are walls: the vertical component turns and the horizontal one is left
 * alone, so a ball crossing the court keeps crossing it.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_bounces_off_the_top_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(400.0f, 10.0f));
    fixture.scene_.ball().direction(glm::vec2(60.0f, -120.0f));

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.ball().direction().x == 60.0f);
    BOOST_TEST(fixture.scene_.ball().direction().y == 120.0f);
    BOOST_TEST(fixture.sounds_.has("bounce"));
}

BOOST_AUTO_TEST_CASE(pong_scene_bounces_off_the_bottom_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(400.0f, 580.0f));
    fixture.scene_.ball().direction(glm::vec2(60.0f, 120.0f));

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.ball().direction().x == 60.0f);
    BOOST_TEST(fixture.scene_.ball().direction().y == -120.0f);
    BOOST_TEST(fixture.sounds_.has("bounce"));
}

/**
 * Both walls turn the ball where its edge meets the wall's face, fifteen pixels in, so the
 * top is no deeper than the bottom.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_walls_turn_the_ball_at_their_faces_test) {
    Fixture top;
    top.scene_.ball().position(glm::vec2(400.0f, 20.0f));
    top.scene_.ball().direction(glm::vec2(60.0f, -120.0f));

    top.scene_.tick(STEP);

    BOOST_TEST(top.scene_.ball().direction().y == 120.0f);

    Fixture clear;
    clear.scene_.ball().position(glm::vec2(400.0f, 21.0f));
    clear.scene_.ball().direction(glm::vec2(60.0f, -120.0f));

    clear.scene_.tick(STEP);

    BOOST_TEST(clear.scene_.ball().direction().y == -120.0f);
    BOOST_TEST(!clear.sounds_.has("bounce"));
}

/**
 * A ball still inside a wall on the step after it was turned is heading out of it, and is
 * not turned back in.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_walls_turn_the_ball_once_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(400.0f, 12.0f));
    fixture.scene_.ball().direction(glm::vec2(60.0f, 120.0f));

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.ball().direction().y == 120.0f);
    BOOST_TEST(!fixture.sounds_.has("bounce"));
}

/**
 * Reaching the target score ends the game rather than the rally: the scene resets and says
 * so, and the next tick is the first of a new round.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_victory_test) {
    Fixture fixture;
    fixture.scene_.left().score(fixture.scene_.state().maxScore());

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.left().score() == 0);
    BOOST_TEST(fixture.scene_.right().score() == 0);
    BOOST_TEST(fixture.sounds_.has("victory"));
}

/**
 * A travelling paddle is moved by the tick, and stops at the ends of its run rather than
 * leaving the court.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paddle_travel_test) {
    Fixture fixture;
    fixture.scene_.left().up(true);

    fixture.scene_.tick(STEP);
    BOOST_TEST(fixture.scene_.left().position() == 298.5f, boost::test_tools::tolerance(0.0001f));

    fixture.scene_.left().up(false);
    fixture.scene_.left().down(true);
    fixture.scene_.tick(STEP);
    BOOST_TEST(fixture.scene_.left().position() == 300.0f, boost::test_tools::tolerance(0.0001f));
}

BOOST_AUTO_TEST_CASE(pong_scene_paddle_travel_is_bounded_test) {
    Fixture fixture;
    fixture.scene_.left().position(40.0f);
    fixture.scene_.left().up(true);

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.left().position() == 40.0f);

    fixture.scene_.right().position(560.0f);
    fixture.scene_.right().down(true);
    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.right().position() == 560.0f);
}

/**
 * Out of coop the right paddle is played by the scene, and it travels towards the ball it is
 * about to have to return.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_ai_follows_the_ball_test) {
    Fixture fixture;
    fixture.scene_.state().coop(false);
    fixture.scene_.ball().position(glm::vec2(400.0f, 100.0f));
    fixture.scene_.ball().direction(glm::vec2(1.0f, 0.0f));

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.right().up());
    BOOST_TEST(!fixture.scene_.right().down());
    BOOST_TEST(fixture.scene_.right().position() == 298.5f);
}

BOOST_AUTO_TEST_CASE(pong_scene_ai_follows_the_ball_downwards_test) {
    Fixture fixture;
    fixture.scene_.state().coop(false);
    fixture.scene_.ball().position(glm::vec2(400.0f, 500.0f));
    fixture.scene_.ball().direction(glm::vec2(1.0f, 0.0f));

    fixture.scene_.tick(STEP);

    BOOST_TEST(!fixture.scene_.right().up());
    BOOST_TEST(fixture.scene_.right().down());
    BOOST_TEST(fixture.scene_.right().position() == 301.5f);
}

/**
 * When the ball is moving away from the ai's paddle, the paddle stops rather than carrying
 * on towards where the ball was.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_ai_rests_when_the_ball_leaves_test) {
    Fixture fixture;
    fixture.scene_.state().coop(false);
    fixture.scene_.right().up(true);
    fixture.scene_.ball().position(glm::vec2(400.0f, 100.0f));
    fixture.scene_.ball().direction(glm::vec2(-1.0f, 0.0f));

    fixture.scene_.tick(STEP);

    BOOST_TEST(!fixture.scene_.right().up());
    BOOST_TEST(!fixture.scene_.right().down());
    BOOST_TEST(fixture.scene_.right().position() == 300.0f);
}

/**
 * In coop the scene does not touch the right paddle at all - both are played.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_coop_leaves_the_right_paddle_alone_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(400.0f, 100.0f));
    fixture.scene_.ball().direction(glm::vec2(1.0f, 0.0f));

    fixture.scene_.tick(STEP);

    BOOST_TEST(!fixture.scene_.right().up());
    BOOST_TEST(!fixture.scene_.right().down());
    BOOST_TEST(fixture.scene_.right().position() == 300.0f);
}

/**
 * Every speed is per second, so the same simulated duration produces the same result however
 * it is divided into steps: sixty steps of a sixtieth land where a hundred and twenty of a
 * hundred and twentieth do.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_is_frame_rate_independent_test) {
    Fixture slow;
    Fixture fast;
    slow.scene_.state().coop(true);
    fast.scene_.state().coop(true);
    slow.scene_.left().up(true);
    fast.scene_.left().up(true);

    for (int i = 0; i < 60; ++i) {
        slow.scene_.tick(1.0f / 60.0f);
    }
    for (int i = 0; i < 120; ++i) {
        fast.scene_.tick(1.0f / 120.0f);
    }

    BOOST_TEST(near(slow.scene_.ball().position(), fast.scene_.ball().position()));
    BOOST_TEST(slow.scene_.left().position() == fast.scene_.left().position(), boost::test_tools::tolerance(0.01f));
}

/**
 * A paddle key let go while the game is paused stops the paddle, so the paddle does not run on
 * by itself when the menu closes. A key pressed while paused does nothing.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_a_release_while_paused_stops_the_paddle_test) {
    Fixture fixture;

    BOOST_TEST(fixture.scene_.steer("leftPaddleUp", true));
    BOOST_TEST(fixture.scene_.left().up());
    fixture.scene_.state().pause(true);
    BOOST_TEST(fixture.scene_.steer("leftPaddleUp", false));
    BOOST_TEST(!fixture.scene_.left().up());

    BOOST_TEST(fixture.scene_.steer("leftPaddleDown", true));
    BOOST_TEST(!fixture.scene_.left().down());
    BOOST_TEST(!fixture.scene_.steer("showGameMenu", true));
}

/**
 * The computer moves the right paddle when a player has not got it. Changing to coop stops it,
 * so the player who takes it over does not find it already moving.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_coop_stops_the_computer_paddle_test) {
    Fixture fixture;
    fixture.scene_.coop(false);
    // a ball headed right and above the paddle, which the computer moves up to meet
    fixture.scene_.ball().direction(glm::vec2(60.0f, 0.0f));
    fixture.scene_.ball().position(glm::vec2(400.0f, 100.0f));
    fixture.scene_.tick(STEP);
    BOOST_REQUIRE(fixture.scene_.right().up());

    fixture.scene_.coop(true);
    fixture.scene_.reset();
    BOOST_TEST(!fixture.scene_.right().up());
    BOOST_TEST(!fixture.scene_.right().down());
}

/**
 * A reset starts a new round and leaves a paused game paused, so a mode chosen from the open
 * menu does not set the ball moving behind it.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_reset_keeps_the_pause_test) {
    Fixture fixture;
    fixture.scene_.state().pause(true);
    fixture.scene_.reset();
    BOOST_TEST(fixture.scene_.state().paused());
}
