/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <memory>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>

#include "../System.h"
#include "../component/Position1D.h"

namespace {

/**
 * A system holds the registry it was given rather than one of its own, because the app
 * owns the registry and passes it in as a raw pointer.
 **/
class CountingSystem final : public v3d::ecs::System {
 public:
    using System::System;

    bool tick() override {
        ticks_++;
        registry_->view<v3d::ecs::component::Position1D>().each(
            [](v3d::ecs::component::Position1D& position) {
                position.set(position.value() + 1.0f);
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

    bool tick() override {
        return false;
    }
};

};  // namespace

/**
 * A tick reaches every entity carrying the component it views, and nothing else.
 **/
BOOST_AUTO_TEST_CASE(system_tick_test) {
    entt::registry registry;
    const entt::entity moving = registry.create();
    const entt::entity still = registry.create();
    registry.emplace<v3d::ecs::component::Position1D>(moving, 0.0f);

    CountingSystem system(&registry);
    BOOST_TEST(system.ticks() == 0);

    BOOST_TEST(system.tick());
    BOOST_TEST(system.tick());

    BOOST_TEST(system.ticks() == 2);
    BOOST_TEST(registry.get<v3d::ecs::component::Position1D>(moving).value() == 2.0f);
    BOOST_TEST(!registry.try_get<v3d::ecs::component::Position1D>(still));
}

/**
 * The return is what an engine's tick loop reads to stop, so a system that fails has to be
 * able to say so through the base.
 **/
BOOST_AUTO_TEST_CASE(system_tick_failure_test) {
    entt::registry registry;
    const std::unique_ptr<v3d::ecs::System> system =
        std::make_unique<FailingSystem>(&registry);

    BOOST_TEST(!system->tick());
}
