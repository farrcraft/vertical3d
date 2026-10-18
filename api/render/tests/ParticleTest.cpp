/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ecs/component/Emitter.h>
#include <api/ecs/component/Transform.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/canvas/DepthOrder.h>
#include <api/render/realtime/entity/Particles.h>
#include <api/render/realtime/entity/Sprites.h>
#include <api/render/realtime/canvas/World.h>
#include <api/render/realtime/component/Particles.h>
#include <api/render/realtime/component/Sprite.h>
#include <api/type/animation/SpriteClip.h>
#include <api/type/animation/Track.h>
#include <api/type/effect/Emitter.h>
#include <api/type/effect/Particle.h>

#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

using v3d::ecs::component::Emitter;
using v3d::render::realtime::canvas::DepthOrder;
using v3d::render::realtime::TextureHandle;
using v3d::render::realtime::component::Particles;
using v3d::type::animation::SpriteClip;
using v3d::type::animation::Track;
using v3d::type::effect::Particle;

namespace canvas = v3d::render::realtime::canvas;

namespace {

constexpr glm::vec3 right(1.0F, 0.0F, 0.0F);
constexpr glm::vec3 up(0.0F, 1.0F, 0.0F);
constexpr glm::vec3 away(0.0F, 0.0F, 1.0F);

bool same(const glm::vec3& first, const glm::vec3& second) {
    return glm::all(glm::epsilonEqual(first, second, 0.0001F));
}

/**
 * A particle a second into a four second life, moving from previous to position.
 **/
Particle particle(const glm::vec3& previous, const glm::vec3& position, const glm::vec3& velocity) {
    Particle made;
    made.previous = previous;
    made.position = position;
    made.velocity = velocity;
    made.age = 1.0F;
    made.lifetime = 4.0F;
    made.phase = 0.0F;
    return made;
}

/**
 * An entity whose emitter holds the given particles, drawn with the given look.
 **/
Emitter& emitting(entt::registry* registry, const std::vector<Particle>& made, const Particles& look) {
    const entt::entity entity = registry->create();
    Emitter& emitter = registry->emplace<Emitter>(entity);
    emitter.state.particles = made;
    registry->emplace<Particles>(entity, look);
    return emitter;
}

void draw(const entt::registry& registry, float alpha, canvas::World* canvas) {
    DepthOrder order;
    v3d::render::realtime::entity::particles(registry, alpha, {.right = right, .up = up, .depthAxis = away}, &order);
    order.into(canvas);
}

BOOST_AUTO_TEST_SUITE(particle_test)

/**
 * A particle is drawn alpha of the way between its steps, as a square centred there, spanned by
 * the camera's right and up, top-left first, at the size its track gives at its life.
 **/
BOOST_AUTO_TEST_CASE(a_particle_is_drawn_between_steps) {
    entt::registry registry;
    Emitter& emitter = emitting(&registry,
        {particle(glm::vec3(0.0F), glm::vec3(4.0F, 0.0F, 0.0F), glm::vec3(0.0F))}, Particles());
    // a quarter of the way through its life, a track from 2 to 6 gives 3
    emitter.description.size = Track<float>({{0.0F, 2.0F}, {1.0F, 6.0F}});

    canvas::World canvas;
    draw(registry, 0.25F, &canvas);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4U);
    const glm::vec3 centre(1.0F, 0.0F, 0.0F);
    BOOST_TEST(same(canvas.vertices()[0].position, centre + glm::vec3(-1.5F, 1.5F, 0.0F)));
    BOOST_TEST(same(canvas.vertices()[1].position, centre + glm::vec3(1.5F, 1.5F, 0.0F)));
    BOOST_TEST(same(canvas.vertices()[2].position, centre + glm::vec3(1.5F, -1.5F, 0.0F)));
    BOOST_TEST(same(canvas.vertices()[3].position, centre + glm::vec3(-1.5F, -1.5F, 0.0F)));
}

/**
 * A particle's colour is its track's at its life, and its region the clip's frame at its age,
 * or at its life when the clip runs over the whole of it.
 **/
BOOST_AUTO_TEST_CASE(a_particle_takes_its_colour_and_frame) {
    const std::vector<SpriteClip::Frame> frames = {
        SpriteClip::Frame{glm::vec2(0.0F, 0.0F), glm::vec2(0.5F, 1.0F), 0.75F},
        SpriteClip::Frame{glm::vec2(0.5F, 0.0F), glm::vec2(1.0F, 1.0F), 0.75F}
    };
    Particles look;
    look.texture = TextureHandle(3);
    look.clip = boost::make_shared<const SpriteClip>(frames, true);

    entt::registry registry;
    Emitter& emitter = emitting(&registry,
        {particle(glm::vec3(0.0F), glm::vec3(0.0F), glm::vec3(0.0F))}, look);
    emitter.description.colour = Track<glm::vec4>({{0.0F, glm::vec4(1.0F)}, {1.0F, glm::vec4(1.0F, 1.0F, 1.0F, 0.0F)}});

    // a second in, by age, the clip is in its second frame
    canvas::World byAge;
    draw(registry, 1.0F, &byAge);
    BOOST_REQUIRE_EQUAL(byAge.vertices().size(), 4U);
    BOOST_TEST((byAge.vertices()[0].uv == glm::vec2(0.5F, 0.0F)));
    BOOST_TEST((byAge.vertices()[0].colour == glm::vec4(1.0F, 1.0F, 1.0F, 0.75F)));
    BOOST_TEST((byAge.batches()[0].texture == TextureHandle(3)));

    // a quarter of the way through its life, over a clip of a second and a half, is the first
    registry.get<Particles>(registry.view<Particles>().front()).overLife = true;
    canvas::World byLife;
    draw(registry, 1.0F, &byLife);
    BOOST_TEST((byLife.vertices()[0].uv == glm::vec2(0.0F, 0.0F)));
}

/**
 * Particles and sprites handed to one order come out furthest first, whichever was added
 * first - a flame between two characters stands between them.
 **/
BOOST_AUTO_TEST_CASE(particles_sort_among_sprites) {
    entt::registry registry;
    emitting(&registry, {particle(glm::vec3(0.0F, 0.0F, 5.0F), glm::vec3(0.0F, 0.0F, 5.0F), glm::vec3(0.0F))}, Particles());
    for (const float depth : {2.0F, 8.0F}) {
        const entt::entity entity = registry.create();
        registry.emplace<v3d::ecs::component::Transform>(entity).position = glm::vec3(0.0F, 0.0F, depth);
        registry.emplace<v3d::render::realtime::component::Sprite>(entity);
    }

    DepthOrder order;
    v3d::render::realtime::entity::particles(registry, 1.0F, {.right = right, .up = up, .depthAxis = away}, &order);
    v3d::render::realtime::entity::sprites(registry, 1.0F, {.right = right, .up = up, .depthAxis = away}, &order);
    canvas::World canvas;
    order.into(&canvas);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 12U);
    BOOST_TEST(canvas.vertices()[0].position.z == 8.0F);
    BOOST_TEST(canvas.vertices()[4].position.z == 5.0F);
    BOOST_TEST(canvas.vertices()[8].position.z == 2.0F);
}

/**
 * A particle stretched along its velocity lies along the velocity as the camera sees it, as long
 * as the travel its stretch spans, and as wide as its size.
 **/
BOOST_AUTO_TEST_CASE(a_stretched_particle_lies_along_its_velocity) {
    Particles look;
    look.facing = Particles::Facing::Velocity;
    look.stretch = 0.5F;

    entt::registry registry;
    // falling at 8 and drifting right at 6, with a part along the view the camera cannot see
    Emitter& emitter = emitting(&registry,
        {particle(glm::vec3(0.0F), glm::vec3(0.0F), glm::vec3(6.0F, -8.0F, 3.0F))}, look);
    emitter.description.size = Track<float>(0.5F);

    canvas::World canvas;
    draw(registry, 1.0F, &canvas);
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4U);

    const glm::vec3 head = (canvas.vertices()[0].position + canvas.vertices()[1].position) * 0.5F;
    const glm::vec3 tail = (canvas.vertices()[2].position + canvas.vertices()[3].position) * 0.5F;
    // ten a second seen, over half a second, is five long
    BOOST_TEST(same(head - tail, glm::vec3(3.0F, -4.0F, 0.0F)));
    BOOST_TEST(glm::length(canvas.vertices()[1].position - canvas.vertices()[0].position) == 0.5F,
        boost::test_tools::tolerance(1e-5F));

    // and one too slow to outrun its width is square
    emitter.state.particles[0].velocity = glm::vec3(0.0F, 0.1F, 0.0F);
    canvas::World slow;
    draw(registry, 1.0F, &slow);
    BOOST_TEST(glm::length(slow.vertices()[0].position - slow.vertices()[3].position) == 0.5F,
        boost::test_tools::tolerance(1e-5F));
}

/**
 * A swaying particle is drawn moved along the camera's right by its sway at its phase, while
 * where it is simulated stays put.
 **/
BOOST_AUTO_TEST_CASE(a_swaying_particle_drifts_where_it_is_drawn) {
    entt::registry registry;
    Particle swaying = particle(glm::vec3(0.0F), glm::vec3(0.0F), glm::vec3(0.0F));
    swaying.age = 0.0F;
    swaying.phase = 0.25F;
    Emitter& emitter = emitting(&registry, {swaying}, Particles());
    emitter.description.sway = 0.5F;
    emitter.description.swayRate = 1.0F;

    canvas::World canvas;
    draw(registry, 1.0F, &canvas);
    // a quarter turn in is the far right of the sway
    BOOST_TEST(same(canvas.vertices()[0].position, glm::vec3(0.5F - 0.5F, 0.5F, 0.0F)));
    BOOST_TEST(same(emitter.state.particles[0].position, glm::vec3(0.0F)));
}

/**
 * An emitter with no look is not drawn, since what it looks like is not known.
 **/
BOOST_AUTO_TEST_CASE(an_emitter_without_a_look_is_not_drawn) {
    entt::registry registry;
    Emitter& emitter = registry.emplace<Emitter>(registry.create());
    emitter.state.particles.push_back(particle(glm::vec3(0.0F), glm::vec3(0.0F), glm::vec3(0.0F)));

    canvas::World canvas;
    draw(registry, 1.0F, &canvas);
    BOOST_TEST(canvas.empty());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
