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
 * A type that can be drawn between two simulation steps. Argument-dependent lookup must find an
 * interpolate(from, to, alpha) for it that returns a T.
 **/
template <typename T>
concept Interpolable = std::copy_constructible<T> && requires(const T& from, const T& to, float alpha) {
    { interpolate(from, to, alpha) } -> std::convertible_to<T>;
};

/**
 * Copy every entity's T into its Previous<T>, giving one to an entity that has none, and take
 * Previous<T> off every entity that no longer has a T.
 *
 * It runs before each simulation step, before anything moves, once for each type a renderer
 * draws between steps. engine::Engine::interpolate<T>() registers a type for the engine to
 * snapshot. Call this directly only for a registry no engine steps.
 *
 * Every entity with a T is snapshotted whether or not it is about to move, so an entity that
 * has stopped is drawn where it stopped. An entity that loses its T and is given one again in a
 * later step is drawn at its new value, not blended from the old one. An entity that loses and
 * regains its T within one step keeps its Previous<T>. Call settle() for it.
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
 * Call it after a teleport writes the new value, such as a ball put back on the centre spot. It
 * copies the entity's current T. Without it, the next frame draws the entity sweeping from its
 * old place to its new one.
 **/
template <typename T>
void settle(entt::registry& registry, entt::entity entity) {
    registry.emplace_or_replace<Previous<T>>(entity, registry.get<T>(entity));
}

/**
 * An entity's T blended alpha of the way from its previous step to its current one.
 *
 * An entity with no Previous<T>, such as one created during the last step, is drawn at its
 * current value.
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
