/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/IsometricController.h>
#include <api/event/Context.h>
#include <api/event/Event.h>
#include <api/event/State.h>
#include <api/type/camera/Isometric.h>

#include <functional>
#include <set>
#include <string>
#include <string_view>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

using v3d::engine::IsometricController;
using v3d::type::camera::Isometric;

namespace {

/**
 * A press of a camera command, or the platform repeating one.
 **/
v3d::event::Event press(const std::string& name, bool repeat) {
    v3d::event::Event event(name, boost::make_shared<v3d::event::Context>("camera"));
    event.state(v3d::event::State::Pressed);
    event.repeat(repeat);
    return event;
}

/**
 * held() over a set of commands the case holds down.
 **/
IsometricController::Held holding(const std::set<std::string, std::less<>>* down) {
    return [down](std::string_view command) { return down->find(command) != down->end(); };
}

bool near(const glm::vec3& actual, const glm::vec3& expected) {
    return glm::length(actual - expected) < 1e-4F;
}

BOOST_AUTO_TEST_SUITE(isometric_controller_test)

/**
 * A pressed rotate command turns the orbit one step, and the platform repeating it does not. A
 * controller that rotated on every event would pass a single press.
 **/
BOOST_AUTO_TEST_CASE(a_rotate_turns_once_per_press) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const std::set<std::string, std::less<>> down;
    Isometric orbit;
    IsometricController controller(&orbit, dispatcher, holding(&down));

    dispatcher->trigger(press("rotate_right", false));
    BOOST_TEST(orbit.azimuth() == 1);
    dispatcher->trigger(press("rotate_right", true));
    dispatcher->trigger(press("rotate_right", true));
    BOOST_TEST(orbit.azimuth() == 1);
    dispatcher->trigger(press("rotate_left", false));
    BOOST_TEST(orbit.azimuth() == 0);
}

/**
 * Two half second steps move the target as far as one step of a second, so movement is measured
 * in seconds. A controller that moved a fixed amount per call would move twice as far.
 **/
BOOST_AUTO_TEST_CASE(a_held_pan_moves_by_time_not_by_steps) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const std::set<std::string, std::less<>> down{"camera::pan_right"};

    Isometric halves;
    IsometricController halving(&halves, dispatcher, holding(&down));
    halving.simulate(0.5F);
    halving.simulate(0.5F);

    Isometric whole;
    IsometricController once(&whole, dispatcher, holding(&down));
    once.simulate(1.0F);

    BOOST_TEST(glm::length(whole.target()) > 1.0F);
    BOOST_TEST(near(halves.target(), whole.target()));
}

/**
 * After a rotate, a pan right moves the target along the rotated right(). At azimuth zero the
 * view's right and a world axis may agree, so the case pans after a turn.
 **/
BOOST_AUTO_TEST_CASE(a_pan_after_a_rotate_follows_the_view) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const std::set<std::string, std::less<>> down{"camera::pan_right"};
    Isometric orbit;
    const glm::vec3 unturned = orbit.right();
    IsometricController::Speeds speeds;
    speeds.pan = 2.0F;
    IsometricController controller(&orbit, dispatcher, holding(&down), IsometricController::Commands(), speeds);

    dispatcher->trigger(press("rotate_right", false));
    BOOST_REQUIRE(!near(orbit.right(), unturned));
    const glm::vec3 before = orbit.target();
    controller.simulate(1.0F);

    BOOST_TEST(near(orbit.target() - before, orbit.right() * 2.0F));
}

/**
 * A held zoom in narrows the view by its speed for each second, and a held zoom out widens it by
 * the same amount. A controller with the two commands swapped would widen on a zoom in.
 *
 * Covers the base: 5fa4cebf added the controller. Checked by swapping zoomIn and zoomOut in
 * simulate(), which fails this case.
 **/
BOOST_AUTO_TEST_CASE(a_held_zoom_in_narrows_and_zoom_out_widens) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    IsometricController::Speeds speeds;
    speeds.zoom = 3.0F;

    const std::set<std::string, std::less<>> in{"camera::zoom_in"};
    Isometric closer;
    IsometricController zoomingIn(&closer, dispatcher, holding(&in), IsometricController::Commands(), speeds);
    zoomingIn.simulate(1.0F);
    BOOST_TEST(closer.zoom() == Isometric::DEFAULT_ZOOM - 3.0F, boost::test_tools::tolerance(1e-5F));

    const std::set<std::string, std::less<>> out{"camera::zoom_out"};
    Isometric wider;
    IsometricController zoomingOut(&wider, dispatcher, holding(&out), IsometricController::Commands(), speeds);
    zoomingOut.simulate(1.0F);
    BOOST_TEST(wider.zoom() == Isometric::DEFAULT_ZOOM + 3.0F, boost::test_tools::tolerance(1e-5F));
}

/**
 * Each pan command moves the target along its own direction: left against right(), up along
 * forward() and down against it. A controller that read one command for two directions would
 * move one of these the wrong way.
 *
 * Covers the base: 5fa4cebf added the controller. Checked by swapping panUp and panDown in
 * simulate(), which fails this case.
 **/
BOOST_AUTO_TEST_CASE(each_pan_command_moves_its_own_way) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    IsometricController::Speeds speeds;
    speeds.pan = 2.0F;
    const auto moved = [&](const std::string& command) {
        const std::set<std::string, std::less<>> down{command};
        Isometric orbit;
        IsometricController controller(&orbit, dispatcher, holding(&down), IsometricController::Commands(), speeds);
        const glm::vec3 before = orbit.target();
        controller.simulate(1.0F);
        return orbit.target() - before;
    };
    const Isometric reference;
    BOOST_TEST(near(moved("camera::pan_left"), reference.right() * -2.0F));
    BOOST_TEST(near(moved("camera::pan_up"), reference.forward() * 2.0F));
    BOOST_TEST(near(moved("camera::pan_down"), reference.forward() * -2.0F));
}

/**
 * A controller that is gone leaves nothing on the dispatcher, so a rotate command after it is
 * destroyed reaches no orbit.
 **/
BOOST_AUTO_TEST_CASE(a_destroyed_controller_stops_listening) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const std::set<std::string, std::less<>> down;
    Isometric orbit;
    {
        IsometricController controller(&orbit, dispatcher, holding(&down));
    }
    dispatcher->trigger(press("rotate_right", false));
    BOOST_TEST(orbit.azimuth() == 0);
}

/**
 * A notch away from the player zooms in by the stated step, a notch toward the player zooms out
 * by it, and the wheel stops at the orbit's clamp. A negative step turns the wheel round.
 **/
BOOST_AUTO_TEST_CASE(the_wheel_zooms_a_step_a_notch) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const std::set<std::string, std::less<>> down;
    IsometricController::Speeds speeds;
    speeds.notch = 2.0F;
    Isometric orbit;
    IsometricController controller(&orbit, dispatcher, holding(&down), IsometricController::Commands(), speeds);

    controller.wheel(1.0F);
    BOOST_TEST(orbit.zoom() == Isometric::DEFAULT_ZOOM - 2.0F, boost::test_tools::tolerance(1e-5F));
    controller.wheel(-1.0F);
    BOOST_TEST(orbit.zoom() == Isometric::DEFAULT_ZOOM, boost::test_tools::tolerance(1e-5F));
    controller.wheel(100.0F);
    BOOST_TEST(orbit.zoom() == Isometric::MINIMUM_ZOOM, boost::test_tools::tolerance(1e-5F));

    speeds.notch = -2.0F;
    Isometric reversed;
    IsometricController turned(&reversed, dispatcher, holding(&down), IsometricController::Commands(), speeds);
    turned.wheel(1.0F);
    BOOST_TEST(reversed.zoom() == Isometric::DEFAULT_ZOOM + 2.0F, boost::test_tools::tolerance(1e-5F));
}

/**
 * With a margin, a cursor at the left edge for one step pans the target against right() by the
 * pan speed times the step, and one at the top pans it along forward(). A cursor in the middle
 * of the window, outside it, or over the ui pans nothing.
 **/
BOOST_AUTO_TEST_CASE(a_cursor_at_an_edge_pans_toward_it) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const std::set<std::string, std::less<>> down;
    const glm::vec2 viewport(800.0F, 600.0F);

    const auto moved = [&](const glm::vec2& at, bool overUi) {
        Isometric orbit;
        IsometricController controller(&orbit, dispatcher, holding(&down));
        controller.edgePan(10.0F);
        controller.cursor(at, viewport, overUi);
        controller.simulate(0.5F);
        return orbit.target() - Isometric().target();
    };

    // the default pan speed for half a second
    const float distance = IsometricController::Speeds().pan * 0.5F;
    const Isometric axes;
    BOOST_TEST(near(moved(glm::vec2(5.0F, 300.0F), false), -axes.right() * distance));
    BOOST_TEST(near(moved(glm::vec2(400.0F, 2.0F), false), axes.forward() * distance));
    BOOST_TEST(near(moved(glm::vec2(799.0F, 300.0F), false), axes.right() * distance));
    BOOST_TEST(near(moved(glm::vec2(400.0F, 300.0F), false), glm::vec3(0.0F)));
    BOOST_TEST(near(moved(glm::vec2(5.0F, 300.0F), true), glm::vec3(0.0F)));
    BOOST_TEST(near(moved(glm::vec2(-5.0F, 300.0F), false), glm::vec3(0.0F)));
}

/**
 * Edge pan is off until it is given a margin, so a controller an app built before it existed
 * does not start panning when the cursor reaches an edge.
 **/
BOOST_AUTO_TEST_CASE(edge_pan_is_off_by_default) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const std::set<std::string, std::less<>> down;
    Isometric orbit;
    IsometricController controller(&orbit, dispatcher, holding(&down));
    controller.cursor(glm::vec2(0.0F, 0.0F), glm::vec2(800.0F, 600.0F), false);
    controller.simulate(1.0F);
    BOOST_TEST(near(orbit.target(), Isometric().target()));
}

/**
 * A key held toward the edge the cursor is at pans at one speed, not at two.
 **/
BOOST_AUTO_TEST_CASE(an_edge_and_a_key_together_pan_at_one_speed) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    const std::set<std::string, std::less<>> down{"camera::pan_right"};
    Isometric orbit;
    IsometricController controller(&orbit, dispatcher, holding(&down));
    controller.edgePan(10.0F);
    controller.cursor(glm::vec2(799.0F, 300.0F), glm::vec2(800.0F, 600.0F), false);
    controller.simulate(1.0F);
    BOOST_TEST(near(orbit.target() - Isometric().target(), Isometric().right() * IsometricController::Speeds().pan));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
