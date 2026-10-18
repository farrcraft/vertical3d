/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/IsometricController.h>
#include <api/event/Context.h>
#include <api/event/Event.h>
#include <api/type/camera/Isometric.h>

#include <functional>
#include <set>
#include <string>
#include <string_view>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>
#include <glm/geometric.hpp>
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

bool near(const glm::vec3& a, const glm::vec3& b) {
    return glm::length(a - b) < 1e-4f;
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
    halving.simulate(0.5f);
    halving.simulate(0.5f);

    Isometric whole;
    IsometricController once(&whole, dispatcher, holding(&down));
    once.simulate(1.0f);

    BOOST_TEST(glm::length(whole.target()) > 1.0f);
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
    speeds.pan = 2.0f;
    IsometricController controller(&orbit, dispatcher, holding(&down), IsometricController::Commands(), speeds);

    dispatcher->trigger(press("rotate_right", false));
    BOOST_REQUIRE(!near(orbit.right(), unturned));
    const glm::vec3 before = orbit.target();
    controller.simulate(1.0f);

    BOOST_TEST(near(orbit.target() - before, orbit.right() * 2.0f));
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
    speeds.zoom = 3.0f;

    const std::set<std::string, std::less<>> in{"camera::zoom_in"};
    Isometric closer;
    IsometricController zoomingIn(&closer, dispatcher, holding(&in), IsometricController::Commands(), speeds);
    zoomingIn.simulate(1.0f);
    BOOST_TEST(closer.zoom() == Isometric::DEFAULT_ZOOM - 3.0f, boost::test_tools::tolerance(1e-5f));

    const std::set<std::string, std::less<>> out{"camera::zoom_out"};
    Isometric wider;
    IsometricController zoomingOut(&wider, dispatcher, holding(&out), IsometricController::Commands(), speeds);
    zoomingOut.simulate(1.0f);
    BOOST_TEST(wider.zoom() == Isometric::DEFAULT_ZOOM + 3.0f, boost::test_tools::tolerance(1e-5f));
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
    speeds.pan = 2.0f;
    const auto moved = [&](const std::string& command) {
        const std::set<std::string, std::less<>> down{command};
        Isometric orbit;
        IsometricController controller(&orbit, dispatcher, holding(&down), IsometricController::Commands(), speeds);
        const glm::vec3 before = orbit.target();
        controller.simulate(1.0f);
        return orbit.target() - before;
    };
    const Isometric reference;
    BOOST_TEST(near(moved("camera::pan_left"), reference.right() * -2.0f));
    BOOST_TEST(near(moved("camera::pan_up"), reference.forward() * 2.0f));
    BOOST_TEST(near(moved("camera::pan_down"), reference.forward() * -2.0f));
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

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
