/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <concepts>

#include <entt/entt.hpp>

namespace v3d::ecs {

/**
 * What an entity's T was at the start of the most recent simulation step. snapshot() writes it,
 * and interpolated() blends from it to the current T.
 **/
template <typename T>
struct Previous final {
    T value;
};

/**
 * A type that can be drawn between two steps: an interpolate(from, to, alpha) beside it,
 * found by argument-dependent lookup, that returns a T.
 **/
template <typename T>
concept Interpolable = std::copy_constructible<T> && requires(const T& from, const T& to, float alpha) {
    { interpolate(from, to, alpha) } -> std::convertible_to<T>;
};

/**
 * Copy every entity's T into its Previous<T>, giving one to an entity that has none, and take
 * Previous<T> off every entity that no longer has a T.
 *
 * Called at the top of simulate(), before anything moves, once for each type a renderer
 * draws between steps. Every entity carrying T is snapshotted whether or not it is about to
 * move, so one that has stopped is drawn where it stopped. An entity that loses its T and is
 * given one again later is drawn at its new value, not blended from the one it lost.
 **/
template <typename T>
void snapshot(entt::registry& registry) {
    for (auto [entity, current] : registry.view<T>().each()) {
        registry.emplace_or_replace<Previous<T>>(entity, current);
    }
    const auto orphaned = registry.view<Previous<T>>(entt::exclude<T>);
    registry.remove<Previous<T>>(orphaned.begin(), orphaned.end());
}

/**
 * Make an entity's previous step its current one, so it is drawn with no motion.
 *
 * A teleport calls this, such as a ball put back on the centre spot, after it has written the
 * new value: this copies the T the entity has now. Otherwise the frame after it draws the
 * entity sweeping from where it was to where it was put.
 **/
template <typename T>
void settle(entt::registry& registry, entt::entity entity) {
    registry.emplace_or_replace<Previous<T>>(entity, registry.get<T>(entity));
}

/**
 * An entity's T blended alpha of the way from its previous step to its current one.
 *
 * An entity with no previous step yet - one created during the step it is first drawn after -
 * is drawn at its current value.
 *
 * @param alpha Engine::alpha(), the fraction of a step elapsed since the last one
 **/
template <typename T>
T interpolated(const entt::registry& registry, entt::entity entity, float alpha) {
    static_assert(Interpolable<T>, "T needs an interpolate(const T&, const T&, float) beside it to be drawn between steps");
    const T& current = registry.get<T>(entity);
    const Previous<T>* previous = registry.try_get<Previous<T>>(entity);
    if (previous == nullptr) {
        return current;
    }
    return interpolate(previous->value, current, alpha);
}

};  // namespace v3d::ecs
