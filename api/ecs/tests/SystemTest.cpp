/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ecs/System.h>

#include "Positions.h"

#include <memory>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>

namespace {

/**
 * A system works on the registry it is given. The app owns the registry and passes it in as a
 * raw pointer.
 **/
class CountingSystem final : public v3d::ecs::System {
 public:
    using System::System;

    bool simulate(float /* step */) override {
        ticks_++;
        registry()->view<positions::Position1D>().each(
            [](positions::Position1D& position) {
                position.value = position.value + 1.0F;
            });
        return true;
    }

    int ticks() const {
        return ticks_;
    }

 private:
    int ticks_ = 0;
};

class FailingSystem final : public v3d::ecs::System {
 public:
    using System::System;

    bool simulate(float /* step */) override {
        return false;
    }
};

/**
 * A step reaches every entity carrying the component it views, and nothing else.
 **/
BOOST_AUTO_TEST_CASE(system_simulate_test) {
    entt::registry registry;
    const entt::entity moving = registry.create();
    const entt::entity still = registry.create();
    registry.emplace<positions::Position1D>(moving, 0.0F);

    CountingSystem system(&registry);
    BOOST_TEST(system.ticks() == 0);

    BOOST_TEST(system.simulate(1.0F / 60.0F));
    BOOST_TEST(system.simulate(1.0F / 60.0F));

    BOOST_TEST(system.ticks() == 2);
    BOOST_TEST(registry.get<positions::Position1D>(moving).value == 2.0F);
    BOOST_TEST(!registry.try_get<positions::Position1D>(still));
}

/**
 * A system reports a failure by returning false, through a pointer to the base. The engine's
 * loop stops when simulate() returns false.
 **/
BOOST_AUTO_TEST_CASE(system_simulate_failure_test) {
    entt::registry registry;
    const std::unique_ptr<v3d::ecs::System> system =
        std::make_unique<FailingSystem>(&registry);

    BOOST_TEST(!system->simulate(1.0F / 60.0F));
}

};  // namespace
