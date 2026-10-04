/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ecs/component/Emitter.h>
#include <api/ecs/component/Transform.h>
#include <api/render/realtime/DepthOrder.h>
#include <api/render/realtime/Particles.h>
#include <api/render/realtime/Sprites.h>
#include <api/render/realtime/WorldCanvas.h>
#include <api/render/realtime/component/Particles.h>
#include <api/render/realtime/component/Sprite.h>
#include <api/type/animation/SpriteClip.h>
#include <api/type/animation/Track.h>
#include <api/type/effect/Emitter.h>

#include <cstddef>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

using v3d::ecs::component::Emitter;
using v3d::render::realtime::DepthOrder;
using v3d::render::realtime::TextureHandle;
using v3d::render::realtime::WorldCanvas;
using v3d::render::realtime::component::Particles;
using v3d::type::animation::SpriteClip;
using v3d::type::animation::Track;
using v3d::type::effect::Particle;

namespace {

constexpr glm::vec3 right(1.0f, 0.0f, 0.0f);
constexpr glm::vec3 up(0.0f, 1.0f, 0.0f);
constexpr glm::vec3 away(0.0f, 0.0f, 1.0f);

bool same(const glm::vec3& a, const glm::vec3& b) {
    return glm::all(glm::epsilonEqual(a, b, 0.0001f));
}

/**
 * A particle a second into a four second life, moving from previous to position.
 **/
Particle particle(const glm::vec3& previous, const glm::vec3& position, const glm::vec3& velocity) {
    Particle made;
    made.previous = previous;
    made.position = position;
    made.velocity = velocity;
    made.age = 1.0f;
    made.lifetime = 4.0f;
    made.phase = 0.0f;
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

void draw(const entt::registry& registry, float alpha, WorldCanvas* canvas) {
    DepthOrder order;
    v3d::render::realtime::particles(registry, alpha, right, up, away, &order);
    order.into(canvas);
}

};  // namespace

BOOST_AUTO_TEST_SUITE(particle_test)

/**
 * A particle is drawn alpha of the way between its steps, as a square centred there, spanned by
 * the camera's right and up, top-left first, at the size its track gives at its life.
 **/
BOOST_AUTO_TEST_CASE(a_particle_is_drawn_between_steps) {
    entt::registry registry;
    Emitter& emitter = emitting(&registry,
        {particle(glm::vec3(0.0f), glm::vec3(4.0f, 0.0f, 0.0f), glm::vec3(0.0f))}, Particles());
    // a quarter of the way through its life, a track from 2 to 6 gives 3
    emitter.description.size = Track<float>({{0.0f, 2.0f}, {1.0f, 6.0f}});

    WorldCanvas canvas;
    draw(registry, 0.25f, &canvas);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4u);
    const glm::vec3 centre(1.0f, 0.0f, 0.0f);
    BOOST_TEST(same(canvas.vertices()[0].position, centre + glm::vec3(-1.5f, 1.5f, 0.0f)));
    BOOST_TEST(same(canvas.vertices()[1].position, centre + glm::vec3(1.5f, 1.5f, 0.0f)));
    BOOST_TEST(same(canvas.vertices()[2].position, centre + glm::vec3(1.5f, -1.5f, 0.0f)));
    BOOST_TEST(same(canvas.vertices()[3].position, centre + glm::vec3(-1.5f, -1.5f, 0.0f)));
}

/**
 * A particle's colour is its track's at its life, and its region the clip's frame at its age,
 * or at its life when the clip runs over the whole of it.
 **/
BOOST_AUTO_TEST_CASE(a_particle_takes_its_colour_and_frame) {
    const std::vector<SpriteClip::Frame> frames = {
        SpriteClip::Frame{glm::vec2(0.0f, 0.0f), glm::vec2(0.5f, 1.0f), 0.75f},
        SpriteClip::Frame{glm::vec2(0.5f, 0.0f), glm::vec2(1.0f, 1.0f), 0.75f}
    };
    Particles look;
    look.texture = TextureHandle(3);
    look.clip = boost::make_shared<const SpriteClip>(frames, true);

    entt::registry registry;
    Emitter& emitter = emitting(&registry,
        {particle(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f))}, look);
    emitter.description.colour = Track<glm::vec4>({{0.0f, glm::vec4(1.0f)}, {1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)}});

    // a second in, by age, the clip is in its second frame
    WorldCanvas byAge;
    draw(registry, 1.0f, &byAge);
    BOOST_REQUIRE_EQUAL(byAge.vertices().size(), 4u);
    BOOST_TEST((byAge.vertices()[0].uv == glm::vec2(0.5f, 0.0f)));
    BOOST_TEST((byAge.vertices()[0].colour == glm::vec4(1.0f, 1.0f, 1.0f, 0.75f)));
    BOOST_TEST((byAge.batches()[0].texture == TextureHandle(3)));

    // a quarter of the way through its life, over a clip of a second and a half, is the first
    registry.get<Particles>(registry.view<Particles>().front()).overLife = true;
    WorldCanvas byLife;
    draw(registry, 1.0f, &byLife);
    BOOST_TEST((byLife.vertices()[0].uv == glm::vec2(0.0f, 0.0f)));
}

/**
 * Particles and sprites handed to one order come out furthest first, whichever was added
 * first - a flame between two characters stands between them.
 **/
BOOST_AUTO_TEST_CASE(particles_sort_among_sprites) {
    entt::registry registry;
    emitting(&registry, {particle(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f))}, Particles());
    for (const float depth : {2.0f, 8.0f}) {
        const entt::entity entity = registry.create();
        registry.emplace<v3d::ecs::component::Transform>(entity).position = glm::vec3(0.0f, 0.0f, depth);
        registry.emplace<v3d::render::realtime::component::Sprite>(entity);
    }

    DepthOrder order;
    v3d::render::realtime::particles(registry, 1.0f, right, up, away, &order);
    v3d::render::realtime::sprites(registry, 1.0f, right, up, away, &order);
    WorldCanvas canvas;
    order.into(&canvas);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 12u);
    BOOST_TEST(canvas.vertices()[0].position.z == 8.0f);
    BOOST_TEST(canvas.vertices()[4].position.z == 5.0f);
    BOOST_TEST(canvas.vertices()[8].position.z == 2.0f);
}

/**
 * A particle stretched along its velocity lies along the velocity as the camera sees it, as long
 * as the travel its stretch spans, and as wide as its size.
 **/
BOOST_AUTO_TEST_CASE(a_stretched_particle_lies_along_its_velocity) {
    Particles look;
    look.facing = Particles::Facing::Velocity;
    look.stretch = 0.5f;

    entt::registry registry;
    // falling at 8 and drifting right at 6, with a part along the view the camera cannot see
    Emitter& emitter = emitting(&registry,
        {particle(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(6.0f, -8.0f, 3.0f))}, look);
    emitter.description.size = Track<float>(0.5f);

    WorldCanvas canvas;
    draw(registry, 1.0f, &canvas);
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4u);

    const glm::vec3 head = (canvas.vertices()[0].position + canvas.vertices()[1].position) * 0.5f;
    const glm::vec3 tail = (canvas.vertices()[2].position + canvas.vertices()[3].position) * 0.5f;
    // ten a second seen, over half a second, is five long
    BOOST_TEST(same(head - tail, glm::vec3(3.0f, -4.0f, 0.0f)));
    BOOST_TEST(glm::length(canvas.vertices()[1].position - canvas.vertices()[0].position) == 0.5f,
        boost::test_tools::tolerance(1e-5f));

    // and one too slow to outrun its width is square
    emitter.state.particles[0].velocity = glm::vec3(0.0f, 0.1f, 0.0f);
    WorldCanvas slow;
    draw(registry, 1.0f, &slow);
    BOOST_TEST(glm::length(slow.vertices()[0].position - slow.vertices()[3].position) == 0.5f,
        boost::test_tools::tolerance(1e-5f));
}

/**
 * A swaying particle is drawn moved along the camera's right by its sway at its phase, while
 * where it is simulated stays put.
 **/
BOOST_AUTO_TEST_CASE(a_swaying_particle_drifts_where_it_is_drawn) {
    entt::registry registry;
    Particle swaying = particle(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    swaying.age = 0.0f;
    swaying.phase = 0.25f;
    Emitter& emitter = emitting(&registry, {swaying}, Particles());
    emitter.description.sway = 0.5f;
    emitter.description.swayRate = 1.0f;

    WorldCanvas canvas;
    draw(registry, 1.0f, &canvas);
    // a quarter turn in is the far right of the sway
    BOOST_TEST(same(canvas.vertices()[0].position, glm::vec3(0.5f - 0.5f, 0.5f, 0.0f)));
    BOOST_TEST(same(emitter.state.particles[0].position, glm::vec3(0.0f)));
}

/**
 * An emitter with no look is not drawn, since what it looks like is not known.
 **/
BOOST_AUTO_TEST_CASE(an_emitter_without_a_look_is_not_drawn) {
    entt::registry registry;
    Emitter& emitter = registry.emplace<Emitter>(registry.create());
    emitter.state.particles.push_back(particle(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)));

    WorldCanvas canvas;
    draw(registry, 1.0f, &canvas);
    BOOST_TEST(canvas.empty());
}

BOOST_AUTO_TEST_SUITE_END()
