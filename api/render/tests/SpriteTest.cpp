/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ecs/Previous.h>
#include <api/ecs/component/Transform.h>
#include <api/render/realtime/DepthOrder.h>
#include <api/render/realtime/Sprites.h>
#include <api/render/realtime/WorldCanvas.h>
#include <api/render/realtime/component/Sprite.h>
#include <api/type/camera/Profile.h>

#include <cstddef>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/vec3.hpp>

using v3d::ecs::component::Transform;
using v3d::render::realtime::DepthOrder;
using v3d::render::realtime::TextureHandle;
using v3d::render::realtime::WorldCanvas;
using v3d::render::realtime::component::Sprite;

namespace {

constexpr glm::vec3 right(1.0f, 0.0f, 0.0f);
constexpr glm::vec3 up(0.0f, 1.0f, 0.0f);
constexpr glm::vec3 away(0.0f, 0.0f, 1.0f);

bool same(const glm::vec3& a, const glm::vec3& b) {
    return glm::all(glm::epsilonEqual(a, b, 0.0001f));
}

entt::entity place(entt::registry* registry, const glm::vec3& feet, const Sprite& sprite) {
    const entt::entity entity = registry->create();
    Transform transform;
    transform.position = feet;
    registry->emplace<Transform>(entity, transform);
    registry->emplace<Sprite>(entity, sprite);
    return entity;
}

Sprite sized(float width, float height, const TextureHandle& texture) {
    Sprite sprite;
    sprite.size = glm::vec2(width, height);
    sprite.texture = texture;
    return sprite;
}

void draw(const entt::registry& registry, float alpha, const glm::vec3& across,
    const glm::vec3& rise, const glm::vec3& depthAxis, WorldCanvas* canvas) {
    DepthOrder order;
    v3d::render::realtime::sprites(registry, alpha, across, rise, depthAxis, &order);
    order.into(canvas);
}

/**
 * The corners of a billboard built by hand, for comparing against what sprites() emits: half
 * the width either side along right, the height along up, top-left first.
 **/
WorldCanvas::Corners billboard(const glm::vec3& feet, float width, float height,
    const glm::vec3& across, const glm::vec3& rise) {
    const glm::vec3 half = across * (width * 0.5f);
    const glm::vec3 top = rise * height;
    return { feet - half + top, feet + half + top, feet + half, feet - half };
}

void checkQuad(const WorldCanvas& canvas, std::size_t n, const WorldCanvas::Corners& corners) {
    for (std::size_t corner = 0; corner < 4; ++corner) {
        BOOST_TEST(same(canvas.vertices()[n * 4 + corner].position, corners[corner]));
    }
}

};  // namespace

BOOST_AUTO_TEST_SUITE(sprite_test)

/**
 * A sprite stands on its feet: the bottom edge is centred on the position and the quad rises
 * along up, top-left first so the region's top-left lands there.
 **/
BOOST_AUTO_TEST_CASE(a_sprite_stands_on_its_feet) {
    entt::registry registry;
    Sprite sprite = sized(2.0f, 3.0f, TextureHandle(1));
    sprite.uv0 = glm::vec2(0.25f, 0.5f);
    sprite.uv1 = glm::vec2(0.75f, 1.0f);
    place(&registry, glm::vec3(0.0f), sprite);

    WorldCanvas canvas;
    draw(registry, 1.0f, right, up, away, &canvas);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4u);
    checkQuad(canvas, 0, {
        glm::vec3(-1.0f, 3.0f, 0.0f),
        glm::vec3(1.0f, 3.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(-1.0f, 0.0f, 0.0f)});
    BOOST_TEST((canvas.vertices()[0].uv == sprite.uv0));
}

/**
 * The transform's scale stretches the quad, x across and y up, and its rotation does not turn
 * it: a billboard faces the camera whichever way the entity faces.
 **/
BOOST_AUTO_TEST_CASE(scale_stretches_and_rotation_does_not_turn) {
    entt::registry registry;
    const entt::entity entity = place(&registry, glm::vec3(0.0f), sized(1.0f, 1.0f, TextureHandle(1)));
    Transform& transform = registry.get<Transform>(entity);
    transform.scale = glm::vec3(2.0f, 4.0f, 1.0f);
    transform.rotation = v3d::ecs::component::aboutY(1.0f);

    WorldCanvas canvas;
    draw(registry, 1.0f, right, up, away, &canvas);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4u);
    checkQuad(canvas, 0, billboard(glm::vec3(0.0f), 2.0f, 4.0f, right, up));
}

/**
 * Sprites sharing a texture are one batch, and the one further along the depth axis is drawn
 * first.
 **/
BOOST_AUTO_TEST_CASE(sprites_are_keyed_along_the_axis) {
    entt::registry registry;
    place(&registry, glm::vec3(0.0f, 0.0f, 1.0f), sized(1.0f, 1.0f, TextureHandle(1)));
    place(&registry, glm::vec3(5.0f, 0.0f, 9.0f), sized(1.0f, 1.0f, TextureHandle(1)));

    WorldCanvas canvas;
    draw(registry, 1.0f, right, up, away, &canvas);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 8u);
    BOOST_CHECK_EQUAL(canvas.batches().size(), 1u);
    BOOST_TEST(same(canvas.vertices()[3].position, glm::vec3(4.5f, 0.0f, 9.0f)));
}

/**
 * An entity with a previous step is drawn between it and its current one, and an entity with
 * none is drawn where it is.
 **/
BOOST_AUTO_TEST_CASE(a_moving_sprite_is_drawn_between_steps) {
    entt::registry registry;
    const entt::entity moving = place(&registry, glm::vec3(0.0f), sized(1.0f, 1.0f, TextureHandle(1)));
    v3d::ecs::snapshot<Transform>(registry);
    registry.get<Transform>(moving).position = glm::vec3(4.0f, 0.0f, 0.0f);
    place(&registry, glm::vec3(0.0f, 0.0f, -10.0f), sized(1.0f, 1.0f, TextureHandle(1)));

    WorldCanvas canvas;
    draw(registry, 0.5f, right, up, away, &canvas);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 8u);
    checkQuad(canvas, 0, billboard(glm::vec3(2.0f, 0.0f, 0.0f), 1.0f, 1.0f, right, up));
    checkQuad(canvas, 1, billboard(glm::vec3(0.0f, 0.0f, -10.0f), 1.0f, 1.0f, right, up));
}

/**
 * A sprite with nowhere to stand is not drawn, and is not an error.
 **/
BOOST_AUTO_TEST_CASE(a_sprite_without_a_transform_is_not_drawn) {
    entt::registry registry;
    registry.emplace<Sprite>(registry.create(), sized(1.0f, 1.0f, TextureHandle(1)));

    WorldCanvas canvas;
    draw(registry, 1.0f, right, up, away, &canvas);

    BOOST_CHECK(canvas.empty());
}

/**
 * A small isometric game world: a 1.6 high marker at the player's feet and a 0.9 high acorn at
 * (2.6, 0, -2.2), on one sheet, under an isometric profile with the eye at (10, 14.142, -10).
 * sprites() must emit the billboard corners built from the profile's right and up, in one
 * batch, keyed along the camera's forward flattened onto the ground.
 *
 * From that eye the acorn is nearer the camera than the player standing at the origin, so the
 * player is drawn first.
 **/
BOOST_AUTO_TEST_CASE(sprite_world_is_drawn_through_sprites) {
    v3d::type::camera::Profile profile("isometric");
    profile.eye(glm::vec3(10.0f, 14.142f, -10.0f));
    profile.lookat(glm::vec3(0.0f));
    const glm::vec3 forward = profile.direction();
    const glm::vec3 ground = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));

    const TextureHandle sheet(3);
    const glm::vec3 player(0.0f);
    const glm::vec3 acorn(2.6f, 0.0f, -2.2f);
    entt::registry registry;
    place(&registry, player, sized(0.8f, 1.6f, sheet));
    place(&registry, acorn, sized(0.9f, 0.9f, sheet));

    WorldCanvas canvas;
    draw(registry, 1.0f, profile.right(), profile.up(), ground, &canvas);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 8u);
    BOOST_CHECK_EQUAL(canvas.batches().size(), 1u);
    checkQuad(canvas, 0, billboard(player, 0.8f, 1.6f, profile.right(), profile.up()));
    checkQuad(canvas, 1, billboard(acorn, 0.9f, 0.9f, profile.right(), profile.up()));
}

BOOST_AUTO_TEST_SUITE_END()
