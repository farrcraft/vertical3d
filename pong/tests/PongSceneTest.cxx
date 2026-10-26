/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ecs/Previous.h>
#include <api/event/kind/Sound.h>
#include <pong/src/PongScene.h>
#include <pong/src/component/Position1D.h>
#include <pong/src/component/Position2D.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/json.hpp>
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
        pans_.push_back(sound.pan());
    }

    /**
     * @return the pan the first sound of a clip was played at, or 0 when none was
     **/
    float pan(const std::string& clip) const {
        const auto found = std::ranges::find(clips_, clip);
        return found == clips_.end() ? 0.0F : pans_[static_cast<std::size_t>(found - clips_.begin())];
    }

    bool has(const std::string& clip) const {
        return std::ranges::any_of(clips_, [&clip](const std::string& played) { return played == clip; });
    }

    std::vector<std::string> clips_;
    std::vector<float> pans_;
};

/**
 * The length of one simulation step, in seconds, as v3d::engine::Accumulator::seconds gives
 * it. Every scene speed below is per second. It is repeated here because this test links
 * neither the engine nor a device.
 **/
constexpr float STEP = 1.0F / 60.0F;

/**
 * Speeds are per second and a sixtieth is not exactly representable, so a position is
 * asserted to within a hundredth of a court unit rather than exactly.
 **/
bool near(const glm::vec2& lhs, const glm::vec2& rhs) {
    return std::abs(lhs.x - rhs.x) < 0.01F && std::abs(lhs.y - rhs.y) < 0.01F;
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

/**
 * reset() centres both paddles and the ball and serves to the left, the state a round
 * begins in.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_reset_test) {
    Fixture fixture;

    BOOST_TEST((fixture.scene_.ball().position() == glm::vec2(400.0F, 300.0F)));
    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(-60.0F, 0.0F)));
    BOOST_TEST(fixture.scene_.ball().size() == 10.0F);
    BOOST_TEST(fixture.scene_.left().position() == 300.0F);
    BOOST_TEST(fixture.scene_.right().position() == 300.0F);
    BOOST_TEST(fixture.scene_.left().score() == 0);
    BOOST_TEST(fixture.scene_.right().score() == 0);
}

/**
 * A tick with nothing in reach moves the ball by one step of its velocity and fires
 * nothing. Sixty units a second over a sixtieth of a second is one unit.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_tick_moves_the_ball_test) {
    Fixture fixture;

    fixture.scene_.tick(STEP);

    BOOST_TEST(near(fixture.scene_.ball().position(), glm::vec2(399.0F, 300.0F)));
    BOOST_TEST(fixture.sounds_.clips_.empty());
}

/**
 * A paused scene does nothing at all. The game is paused while the menu is open.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paused_test) {
    Fixture fixture;
    fixture.scene_.state().pause(true);

    fixture.scene_.tick(STEP);

    BOOST_TEST((fixture.scene_.ball().position() == glm::vec2(400.0F, 300.0F)));
    BOOST_TEST(fixture.sounds_.clips_.empty());
}

/**
 * The paddle test is a box against a box, so the ball is met when it is within half its own
 * size of the paddle's face and within half the paddle's length of its centre.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_left_paddle_collision_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(20.0F, 300.0F));
    fixture.scene_.ball().direction(glm::vec2(-60.0F, 0.0F));

    fixture.scene_.tick(STEP);

    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(60.0F, 0.0F)));
    BOOST_TEST(fixture.sounds_.has("hit"));
}

BOOST_AUTO_TEST_CASE(pong_scene_right_paddle_collision_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(780.0F, 300.0F));
    fixture.scene_.ball().direction(glm::vec2(60.0F, 0.0F));

    fixture.scene_.tick(STEP);

    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(-60.0F, 0.0F)));
    BOOST_TEST(fixture.sounds_.has("hit"));
}

/**
 * A sound is panned to the side of the court it happens on. A hit off the left paddle and a
 * point at the left edge sound from the left, and the same on the right from the right.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_pans_each_sound_to_its_side_test) {
    // where the ball is, and which way it is going
    struct Ball {
        glm::vec2 at{0.0F};
        glm::vec2 heading{0.0F};
    };
    const auto heard = [](const Ball& ball, const std::string& clip) {
        Fixture fixture;
        fixture.scene_.ball().position(ball.at);
        fixture.scene_.ball().direction(ball.heading);
        fixture.scene_.tick(STEP);
        BOOST_TEST_REQUIRE(fixture.sounds_.has(clip));
        return fixture.sounds_.pan(clip);
    };

    BOOST_TEST(heard({.at = glm::vec2(20.0F, 300.0F), .heading = glm::vec2(-60.0F, 0.0F)}, "hit") < -0.9F);
    BOOST_TEST(heard({.at = glm::vec2(780.0F, 300.0F), .heading = glm::vec2(60.0F, 0.0F)}, "hit") > 0.9F);
    BOOST_TEST(heard({.at = glm::vec2(5.0F, 100.0F), .heading = glm::vec2(-60.0F, 0.0F)}, "score") < -0.9F);
    BOOST_TEST(heard({.at = glm::vec2(795.0F, 100.0F), .heading = glm::vec2(60.0F, 0.0F)}, "score") > 0.9F);
}

/**
 * A ball that has already passed the paddle's face, as a fast one does between two steps, is
 * still returned rather than let through to the edge behind the paddle.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paddle_past_the_face_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(8.0F, 300.0F));
    fixture.scene_.ball().direction(glm::vec2(-60.0F, 0.0F));

    fixture.scene_.tick(STEP);

    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(60.0F, 0.0F)));
    BOOST_TEST(fixture.sounds_.has("hit"));

    Fixture right;
    right.scene_.ball().position(glm::vec2(792.0F, 300.0F));
    right.scene_.ball().direction(glm::vec2(60.0F, 0.0F));

    right.scene_.tick(STEP);

    BOOST_TEST((right.scene_.ball().direction() == glm::vec2(-60.0F, 0.0F)));
    BOOST_TEST(right.sounds_.has("hit"));
}

/**
 * A ball level with the paddle's face but past the end of it is not met. This case
 * separates a save from a point.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paddle_miss_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(20.0F, 100.0F));
    fixture.scene_.ball().direction(glm::vec2(-60.0F, 0.0F));

    fixture.scene_.tick(STEP);

    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(-60.0F, 0.0F)));
    BOOST_TEST(!fixture.sounds_.has("hit"));
}

/**
 * A paddle travelling as it meets the ball carries the return the way it is going, on both
 * sides: a paddle moving up sends the ball up, which is towards smaller y.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paddle_travel_angles_the_return_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(20.0F, 300.0F));
    fixture.scene_.ball().direction(glm::vec2(-60.0F, 0.0F));
    fixture.scene_.left().up(true);

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.ball().direction().x > 0.0F);
    BOOST_TEST(fixture.scene_.ball().direction().y < 0.0F);

    Fixture right;
    right.scene_.ball().position(glm::vec2(780.0F, 300.0F));
    right.scene_.ball().direction(glm::vec2(60.0F, 0.0F));
    right.scene_.right().down(true);

    right.scene_.tick(STEP);

    BOOST_TEST(right.scene_.ball().direction().x < 0.0F);
    BOOST_TEST(right.scene_.ball().direction().y > 0.0F);
}

/**
 * A travelling paddle turns the return but does not speed it up. The ball leaves at the speed
 * it arrived at, however many times a moving paddle returns it.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_moving_paddle_keeps_the_speed_test) {
    Fixture fixture;
    fixture.scene_.left().down(true);
    glm::vec2 incoming(-60.0F, 0.0F);
    for (int hit = 0; hit < 3; hit++) {
        fixture.scene_.ball().position(glm::vec2(20.0F, 300.0F));
        fixture.scene_.ball().direction(incoming);

        fixture.scene_.tick(STEP);

        const glm::vec2 returned = fixture.scene_.ball().direction();
        BOOST_TEST(returned.x > 0.0F);
        BOOST_TEST(returned.y > 0.0F);
        BOOST_TEST(glm::length(returned) == 60.0F, boost::test_tools::tolerance(0.001F));
        // the next return meets the ball coming back the way this one left
        incoming = glm::vec2(-returned.x, returned.y);
    }
}

/**
 * Where the ball meets the paddle sets the angle it goes back at: flat off the centre,
 * downwards off the lower half and upwards off the upper, steepest off the ends. The speed
 * is kept, whatever the angle.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paddle_angles_by_where_it_is_struck_test) {
    const float struck[] = { 290.0F, 300.0F, 310.0F, 330.0F };
    float lastAngle = -1.0F;
    for (const float y : struck) {
        Fixture fixture;
        fixture.scene_.ball().position(glm::vec2(20.0F, y));
        fixture.scene_.ball().direction(glm::vec2(-60.0F, 0.0F));

        fixture.scene_.tick(STEP);

        const glm::vec2 returned = fixture.scene_.ball().direction();
        BOOST_TEST(returned.x > 0.0F);
        BOOST_TEST(glm::length(returned) == 60.0F, boost::test_tools::tolerance(0.001F));
        const float angle = std::atan2(returned.y, returned.x);
        BOOST_TEST(angle > lastAngle);
        lastAngle = angle;
    }
    // the last was struck by the very end, at the steepest return: fifty degrees, downwards
    BOOST_TEST(lastAngle == 0.8727F, boost::test_tools::tolerance(0.001F));

    // an angled ball struck by the centre goes back flat rather than retracing its line
    Fixture flat;
    flat.scene_.ball().position(glm::vec2(20.0F, 300.0F));
    flat.scene_.ball().direction(glm::vec2(-48.0F, 36.0F));

    flat.scene_.tick(STEP);

    BOOST_TEST(flat.scene_.ball().direction().x == 60.0F, boost::test_tools::tolerance(0.001F));
    BOOST_TEST(flat.scene_.ball().direction().y == 0.0F, boost::test_tools::tolerance(0.001F));
}

/**
 * A ball already heading away from a paddle is not returned again on the step after it was,
 * however deep past the face it still is.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paddle_returns_once_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(8.0F, 300.0F));
    fixture.scene_.ball().direction(glm::vec2(60.0F, 0.0F));

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.ball().direction().x == 60.0F);
    BOOST_TEST(!fixture.sounds_.has("hit"));
}

/**
 * The ball reaching an edge is a point to the far paddle, and the ball goes back to the
 * middle serving towards whoever conceded it.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_left_edge_scores_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(5.0F, 100.0F));
    fixture.scene_.ball().direction(glm::vec2(-60.0F, 0.0F));

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.right().score() == 1);
    BOOST_TEST(fixture.scene_.left().score() == 0);
    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(-60.0F, 0.0F)));
    BOOST_TEST(fixture.scene_.left().position() == 300.0F);
    BOOST_TEST(fixture.sounds_.has("score"));
}

/**
 * A point puts the ball back on the centre spot rather than moving it there. The frame after
 * the point is drawn from the centre, not swept across the court from the edge it left by.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_a_point_settles_the_ball_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(5.0F, 100.0F));
    fixture.scene_.ball().direction(glm::vec2(-60.0F, 0.0F));
    fixture.scene_.left().position(500.0F);

    v3d::ecs::snapshot<Position2D>(fixture.registry_);
    v3d::ecs::snapshot<Position1D>(fixture.registry_);
    fixture.scene_.tick(STEP);

    BOOST_TEST(near(fixture.scene_.ball().drawn(0.0F), glm::vec2(400.0F, 300.0F)));
    BOOST_TEST(fixture.scene_.left().drawn(0.0F) == 300.0F);
}

/**
 * Between two steps the ball is drawn between where the steps left it.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_ball_drawn_between_steps_test) {
    Fixture fixture;

    v3d::ecs::snapshot<Position2D>(fixture.registry_);
    fixture.scene_.tick(STEP);

    BOOST_TEST(near(fixture.scene_.ball().drawn(0.5F), glm::vec2(399.5F, 300.0F)));
}

BOOST_AUTO_TEST_CASE(pong_scene_right_edge_scores_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(795.0F, 100.0F));
    fixture.scene_.ball().direction(glm::vec2(60.0F, 0.0F));

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.left().score() == 1);
    BOOST_TEST(fixture.scene_.right().score() == 0);
    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(60.0F, 0.0F)));
    BOOST_TEST(fixture.sounds_.has("score"));
}

/**
 * Top and bottom are walls: the vertical component turns and the horizontal one is left
 * alone, so a ball crossing the court keeps crossing it.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_bounces_off_the_top_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(400.0F, 10.0F));
    fixture.scene_.ball().direction(glm::vec2(60.0F, -120.0F));

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.ball().direction().x == 60.0F);
    BOOST_TEST(fixture.scene_.ball().direction().y == 120.0F);
    BOOST_TEST(fixture.sounds_.has("bounce"));
}

BOOST_AUTO_TEST_CASE(pong_scene_bounces_off_the_bottom_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(400.0F, 580.0F));
    fixture.scene_.ball().direction(glm::vec2(60.0F, 120.0F));

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.ball().direction().x == 60.0F);
    BOOST_TEST(fixture.scene_.ball().direction().y == -120.0F);
    BOOST_TEST(fixture.sounds_.has("bounce"));
}

/**
 * Both walls turn the ball where its edge meets the wall's face, fifteen court units in, so the
 * top is no deeper than the bottom.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_walls_turn_the_ball_at_their_faces_test) {
    Fixture top;
    top.scene_.ball().position(glm::vec2(400.0F, 20.0F));
    top.scene_.ball().direction(glm::vec2(60.0F, -120.0F));

    top.scene_.tick(STEP);

    BOOST_TEST(top.scene_.ball().direction().y == 120.0F);

    Fixture clear;
    clear.scene_.ball().position(glm::vec2(400.0F, 21.0F));
    clear.scene_.ball().direction(glm::vec2(60.0F, -120.0F));

    clear.scene_.tick(STEP);

    BOOST_TEST(clear.scene_.ball().direction().y == -120.0F);
    BOOST_TEST(!clear.sounds_.has("bounce"));
}

/**
 * A ball still inside a wall on the step after it was turned is heading out of it, and is
 * not turned back in.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_walls_turn_the_ball_once_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(400.0F, 12.0F));
    fixture.scene_.ball().direction(glm::vec2(60.0F, 120.0F));

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.ball().direction().y == 120.0F);
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
 * Every clip the scene plays is listed in pong's sounds document. A clip missing from it would
 * be silent, and logged as a clip that cannot be played.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_plays_only_clips_its_sounds_document_holds_test) {
    const std::ifstream file("data/sounds.json");
    BOOST_REQUIRE(file.is_open());
    std::stringstream text;
    text << file.rdbuf();
    const boost::json::object document = boost::json::parse(text.str()).as_object();
    std::vector<std::string> held;
    for (const boost::json::value& sound : document.at("sounds").as_array()) {
        held.emplace_back(sound.as_object().at("clip_id").as_string().c_str());
    }

    Fixture fixture;
    // a paddle hit
    fixture.scene_.ball().position(glm::vec2(20.0F, 300.0F));
    fixture.scene_.ball().direction(glm::vec2(-60.0F, 0.0F));
    fixture.scene_.tick(STEP);
    // a wall bounce
    fixture.scene_.ball().position(glm::vec2(400.0F, 10.0F));
    fixture.scene_.ball().direction(glm::vec2(60.0F, -120.0F));
    fixture.scene_.tick(STEP);
    // a point
    fixture.scene_.ball().position(glm::vec2(5.0F, 100.0F));
    fixture.scene_.ball().direction(glm::vec2(-60.0F, 0.0F));
    fixture.scene_.tick(STEP);
    // a win
    fixture.scene_.left().score(fixture.scene_.state().maxScore());
    fixture.scene_.tick(STEP);

    for (const char* clip : { "hit", "bounce", "score", "victory" }) {
        BOOST_TEST_CONTEXT(clip) {
            BOOST_CHECK(fixture.sounds_.has(clip));
        }
    }
    for (const std::string& clip : fixture.sounds_.clips_) {
        BOOST_TEST_CONTEXT(clip) {
            BOOST_CHECK(std::ranges::find(held, clip) != held.end());
        }
    }
}

/**
 * A travelling paddle is moved by the tick, and stops at the ends of its run rather than
 * leaving the court.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paddle_travel_test) {
    Fixture fixture;
    fixture.scene_.left().up(true);

    fixture.scene_.tick(STEP);
    BOOST_TEST(fixture.scene_.left().position() == 298.5F, boost::test_tools::tolerance(0.0001F));

    fixture.scene_.left().up(false);
    fixture.scene_.left().down(true);
    fixture.scene_.tick(STEP);
    BOOST_TEST(fixture.scene_.left().position() == 300.0F, boost::test_tools::tolerance(0.0001F));
}

BOOST_AUTO_TEST_CASE(pong_scene_paddle_travel_is_bounded_test) {
    Fixture fixture;
    fixture.scene_.left().position(40.0F);
    fixture.scene_.left().up(true);

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.left().position() == 40.0F);

    fixture.scene_.right().position(560.0F);
    fixture.scene_.right().down(true);
    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.right().position() == 560.0F);
}

/**
 * A paddle less than a step from the end of its run stops at the end rather than carrying the
 * whole step into the wall. A step is one and a half units, and each paddle starts one unit in.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paddle_stops_at_the_wall_test) {
    Fixture fixture;
    fixture.scene_.left().position(41.0F);
    fixture.scene_.left().up(true);
    fixture.scene_.right().position(559.0F);
    fixture.scene_.right().down(true);

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.left().position() == 40.0F);
    BOOST_TEST(fixture.scene_.right().position() == 560.0F);
}

/**
 * Out of coop the right paddle is played by the scene, and it travels towards the ball it is
 * about to have to return.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_ai_follows_the_ball_test) {
    Fixture fixture;
    fixture.scene_.state().coop(false);
    fixture.scene_.ball().position(glm::vec2(400.0F, 100.0F));
    fixture.scene_.ball().direction(glm::vec2(1.0F, 0.0F));

    fixture.scene_.tick(STEP);

    BOOST_TEST(fixture.scene_.right().up());
    BOOST_TEST(!fixture.scene_.right().down());
    BOOST_TEST(fixture.scene_.right().position() == 298.5F);
}

BOOST_AUTO_TEST_CASE(pong_scene_ai_follows_the_ball_downwards_test) {
    Fixture fixture;
    fixture.scene_.state().coop(false);
    fixture.scene_.ball().position(glm::vec2(400.0F, 500.0F));
    fixture.scene_.ball().direction(glm::vec2(1.0F, 0.0F));

    fixture.scene_.tick(STEP);

    BOOST_TEST(!fixture.scene_.right().up());
    BOOST_TEST(fixture.scene_.right().down());
    BOOST_TEST(fixture.scene_.right().position() == 301.5F);
}

/**
 * When the ball is moving away from the ai's paddle, the paddle stops rather than carrying
 * on towards where the ball was.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_ai_rests_when_the_ball_leaves_test) {
    Fixture fixture;
    fixture.scene_.state().coop(false);
    fixture.scene_.right().up(true);
    fixture.scene_.ball().position(glm::vec2(400.0F, 100.0F));
    fixture.scene_.ball().direction(glm::vec2(-1.0F, 0.0F));

    fixture.scene_.tick(STEP);

    BOOST_TEST(!fixture.scene_.right().up());
    BOOST_TEST(!fixture.scene_.right().down());
    BOOST_TEST(fixture.scene_.right().position() == 300.0F);
}

/**
 * In coop the scene does not touch the right paddle at all - both are played.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_coop_leaves_the_right_paddle_alone_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(400.0F, 100.0F));
    fixture.scene_.ball().direction(glm::vec2(1.0F, 0.0F));

    fixture.scene_.tick(STEP);

    BOOST_TEST(!fixture.scene_.right().up());
    BOOST_TEST(!fixture.scene_.right().down());
    BOOST_TEST(fixture.scene_.right().position() == 300.0F);
}

/**
 * Every speed is per second, so the same simulated duration produces the same result however
 * it is divided into steps. Sixty steps of a sixtieth land where a hundred and twenty steps of
 * a hundred and twentieth do.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_is_frame_rate_independent_test) {
    Fixture slow;
    Fixture fast;
    slow.scene_.state().coop(true);
    fast.scene_.state().coop(true);
    slow.scene_.left().up(true);
    fast.scene_.left().up(true);

    for (int i = 0; i < 60; ++i) {
        slow.scene_.tick(1.0F / 60.0F);
    }
    for (int i = 0; i < 120; ++i) {
        fast.scene_.tick(1.0F / 120.0F);
    }

    BOOST_TEST(near(slow.scene_.ball().position(), fast.scene_.ball().position()));
    BOOST_TEST(slow.scene_.left().position() == fast.scene_.left().position(), boost::test_tools::tolerance(0.01F));
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
    fixture.scene_.ball().direction(glm::vec2(60.0F, 0.0F));
    fixture.scene_.ball().position(glm::vec2(400.0F, 100.0F));
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

/**
 * Outside coop the computer steers the right paddle, so the right paddle's keys, read every
 * step whether held or not, do not stop it.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_keys_leave_the_computer_paddle_alone_test) {
    Fixture fixture;
    fixture.scene_.coop(false);
    fixture.scene_.right().up(true);
    BOOST_TEST(fixture.scene_.steer("rightPaddleUp", false));
    BOOST_TEST(fixture.scene_.right().up());
}

};  // namespace
