/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "../src/PongScene.h"
#include "../../api/event/Sound.h"

namespace {

/**
 * The scene says what happened by triggering a sound, so the clips a tick fires are the
 * only account of which branch it took.
 **/
struct Sounds final {
    void heard(const v3d::event::Sound& sound) {
        clips_.push_back(std::string(sound.clip()));
    }

    bool has(const std::string& clip) const {
        for (const auto& played : clips_) {
            if (played == clip) {
                return true;
            }
        }
        return false;
    }

    std::vector<std::string> clips_;
};

/**
 * A scene the size of pong's own window, reset and listening. Everything below measures
 * against the 800x600 the app opens at, since the collision tests are written in pixels.
 **/
struct Fixture final {
    Fixture() :
        dispatcher_(boost::make_shared<entt::dispatcher>()),
        scene_(&registry_, dispatcher_) {
        dispatcher_->sink<v3d::event::Sound>().connect<&Sounds::heard>(sounds_);
        scene_.resize(800, 600);
        scene_.reset();
    }

    entt::registry registry_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    Sounds sounds_;
    PongScene scene_;
};

};  // namespace

/**
 * reset() centres both paddles and the ball and serves to the left, which is the state a
 * round begins in.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_reset_test) {
    Fixture fixture;

    BOOST_TEST((fixture.scene_.ball().position() == glm::vec2(400.0f, 300.0f)));
    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(-1.0f, 0.0f)));
    BOOST_TEST(fixture.scene_.ball().size() == 10.0f);
    BOOST_TEST(fixture.scene_.left().position() == 300.0f);
    BOOST_TEST(fixture.scene_.right().position() == 300.0f);
    BOOST_TEST(fixture.scene_.left().score() == 0);
    BOOST_TEST(fixture.scene_.right().score() == 0);
}

/**
 * A tick with nothing in reach moves the ball by its direction and fires nothing.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_tick_moves_the_ball_test) {
    Fixture fixture;

    fixture.scene_.tick();

    BOOST_TEST((fixture.scene_.ball().position() == glm::vec2(399.0f, 300.0f)));
    BOOST_TEST(fixture.sounds_.clips_.empty());
}

/**
 * A paused scene does nothing at all, which is what the menu holds the game on.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paused_test) {
    Fixture fixture;
    fixture.scene_.state().pause(true);

    fixture.scene_.tick();

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
    fixture.scene_.ball().direction(glm::vec2(-1.0f, 0.0f));

    fixture.scene_.tick();

    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(1.0f, 0.0f)));
    BOOST_TEST(fixture.sounds_.has("hit"));
}

BOOST_AUTO_TEST_CASE(pong_scene_right_paddle_collision_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(780.0f, 300.0f));
    fixture.scene_.ball().direction(glm::vec2(1.0f, 0.0f));

    fixture.scene_.tick();

    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(-1.0f, 0.0f)));
    BOOST_TEST(fixture.sounds_.has("hit"));
}

/**
 * A ball level with the paddle's face but past the end of it is not met, which is the case
 * that separates a save from a point.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paddle_miss_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(20.0f, 100.0f));
    fixture.scene_.ball().direction(glm::vec2(-1.0f, 0.0f));

    fixture.scene_.tick();

    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(-1.0f, 0.0f)));
    BOOST_TEST(!fixture.sounds_.has("hit"));
}

/**
 * A paddle travelling as it meets the ball puts a little of that travel into the return,
 * which is the only way a rally changes angle.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_paddle_travel_angles_the_return_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(20.0f, 300.0f));
    fixture.scene_.ball().direction(glm::vec2(-1.0f, 0.0f));
    fixture.scene_.left().up(true);

    fixture.scene_.tick();

    BOOST_TEST(fixture.scene_.ball().direction().x == 1.0f);
    BOOST_TEST(fixture.scene_.ball().direction().y == 0.015f);
}

/**
 * The ball reaching an edge is a point to the far paddle, and the ball goes back to the
 * middle serving towards whoever conceded it.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_left_edge_scores_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(5.0f, 100.0f));
    fixture.scene_.ball().direction(glm::vec2(-1.0f, 0.0f));

    fixture.scene_.tick();

    BOOST_TEST(fixture.scene_.right().score() == 1);
    BOOST_TEST(fixture.scene_.left().score() == 0);
    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(-1.0f, 0.0f)));
    BOOST_TEST(fixture.scene_.left().position() == 300.0f);
    BOOST_TEST(fixture.sounds_.has("score"));
}

BOOST_AUTO_TEST_CASE(pong_scene_right_edge_scores_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(795.0f, 100.0f));
    fixture.scene_.ball().direction(glm::vec2(1.0f, 0.0f));

    fixture.scene_.tick();

    BOOST_TEST(fixture.scene_.left().score() == 1);
    BOOST_TEST(fixture.scene_.right().score() == 0);
    BOOST_TEST((fixture.scene_.ball().direction() == glm::vec2(1.0f, 0.0f)));
    BOOST_TEST(fixture.sounds_.has("score"));
}

/**
 * Top and bottom are walls: the vertical component turns and the horizontal one is left
 * alone, so a ball crossing the court keeps crossing it.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_bounces_off_the_top_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(400.0f, 10.0f));
    fixture.scene_.ball().direction(glm::vec2(1.0f, -2.0f));

    fixture.scene_.tick();

    BOOST_TEST(fixture.scene_.ball().direction().x == 1.0f);
    BOOST_TEST(fixture.scene_.ball().direction().y == 2.0f);
    BOOST_TEST(fixture.sounds_.has("bounce"));
}

BOOST_AUTO_TEST_CASE(pong_scene_bounces_off_the_bottom_test) {
    Fixture fixture;
    fixture.scene_.ball().position(glm::vec2(400.0f, 580.0f));
    fixture.scene_.ball().direction(glm::vec2(1.0f, 2.0f));

    fixture.scene_.tick();

    BOOST_TEST(fixture.scene_.ball().direction().x == 1.0f);
    BOOST_TEST(fixture.scene_.ball().direction().y == -2.0f);
    BOOST_TEST(fixture.sounds_.has("bounce"));
}

/**
 * Reaching the target score ends the game rather than the rally: the scene resets and says
 * so, and the next tick is the first of a new round.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_victory_test) {
    Fixture fixture;
    fixture.scene_.left().score(fixture.scene_.state().maxScore());

    fixture.scene_.tick();

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

    fixture.scene_.tick();
    BOOST_TEST(fixture.scene_.left().position() == 298.5f);

    fixture.scene_.left().up(false);
    fixture.scene_.left().down(true);
    fixture.scene_.tick();
    BOOST_TEST(fixture.scene_.left().position() == 300.0f);
}

BOOST_AUTO_TEST_CASE(pong_scene_paddle_travel_is_bounded_test) {
    Fixture fixture;
    fixture.scene_.left().position(40.0f);
    fixture.scene_.left().up(true);

    fixture.scene_.tick();

    BOOST_TEST(fixture.scene_.left().position() == 40.0f);

    fixture.scene_.right().position(560.0f);
    fixture.scene_.right().down(true);
    fixture.scene_.tick();

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

    fixture.scene_.tick();

    BOOST_TEST(fixture.scene_.right().up());
    BOOST_TEST(!fixture.scene_.right().down());
    BOOST_TEST(fixture.scene_.right().position() == 298.5f);
}

BOOST_AUTO_TEST_CASE(pong_scene_ai_follows_the_ball_downwards_test) {
    Fixture fixture;
    fixture.scene_.state().coop(false);
    fixture.scene_.ball().position(glm::vec2(400.0f, 500.0f));
    fixture.scene_.ball().direction(glm::vec2(1.0f, 0.0f));

    fixture.scene_.tick();

    BOOST_TEST(!fixture.scene_.right().up());
    BOOST_TEST(fixture.scene_.right().down());
    BOOST_TEST(fixture.scene_.right().position() == 301.5f);
}

/**
 * A ball going the other way is not the ai's problem, so it stops rather than carrying on
 * towards where the ball was.
 **/
BOOST_AUTO_TEST_CASE(pong_scene_ai_rests_when_the_ball_leaves_test) {
    Fixture fixture;
    fixture.scene_.state().coop(false);
    fixture.scene_.right().up(true);
    fixture.scene_.ball().position(glm::vec2(400.0f, 100.0f));
    fixture.scene_.ball().direction(glm::vec2(-1.0f, 0.0f));

    fixture.scene_.tick();

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

    fixture.scene_.tick();

    BOOST_TEST(!fixture.scene_.right().up());
    BOOST_TEST(!fixture.scene_.right().down());
    BOOST_TEST(fixture.scene_.right().position() == 300.0f);
}
