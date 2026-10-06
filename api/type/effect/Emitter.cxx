/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Emitter.h"

#include "Particle.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

namespace v3d::type::effect {

namespace {

/**
 * How far below a whole particle the owed fraction may fall and still be spawned. A rate summed
 * a step at a time drifts below the whole number it should reach, and without this a rate of
 * five a second at sixty steps spawns four in the first second.
 **/
const float owedSlack = 1e-4f;

/**
 * The most particles one step can owe: the largest float below 2^32, so that the count still
 * fits the unsigned it is returned as. An emitter's cap keeps far fewer than this alive.
 **/
const float mostOwed = 4294967040.0f;

/**
 * Where a particle is born, in the emitter's own space.
 **/
glm::vec3 birthplace(const Emitter& emitter, Random* random) {
    switch (emitter.shape) {
        case Emitter::Shape::Sphere: {
            const glm::vec3 direction = random->cone(glm::vec3(0.0f, 1.0f, 0.0f), glm::pi<float>());
            // a radius drawn by the cube root spreads points evenly through the ball's volume
            return direction * (emitter.extent.x * std::cbrt(random->unit()));
        }
        case Emitter::Shape::Box:
            return random->inside(-emitter.extent, emitter.extent);
        case Emitter::Shape::Point:
        default:
            return glm::vec3(0.0f);
    }
}

};  // namespace

void travel(const Emitter& emitter, State* state, float seconds, const glm::vec3& wind) {
    std::vector<Particle>& particles = state->particles;
    const glm::vec3 acceleration = emitter.acceleration + wind;
    const float kept = std::max(0.0f, 1.0f - (emitter.drag * seconds));
    std::size_t index = 0;
    while (index < particles.size()) {
        Particle& particle = particles[index];
        particle.age += seconds;
        if (particle.age >= particle.lifetime) {
            particle = particles.back();
            particles.pop_back();
            continue;
        }
        particle.previous = particle.position;
        particle.velocity = (particle.velocity + (acceleration * seconds)) * kept;
        particle.position += particle.velocity * seconds;
        index++;
    }
}

uint32_t owing(State* state, float rate, float seconds) {
    // a rate below nothing earns nothing, and so does one that is not a finite number, rather
    // than a count that wraps round or is undefined when it is made unsigned. What is owed is
    // capped at the largest count a step can return
    const float earned = rate * seconds;
    if (std::isfinite(earned) && earned > 0.0f) {
        state->owed = std::min(state->owed + earned, mostOwed);
    }
    const float whole = std::floor(state->owed + owedSlack);
    state->owed = std::max(0.0f, state->owed - whole);
    return static_cast<uint32_t>(whole);
}

bool spawn(const Emitter& emitter, State* state, const glm::vec3& position, const glm::quat& orientation) {
    if (state->particles.size() >= emitter.cap) {
        return false;
    }
    const glm::vec3 heading = emitter.spread > 0.0f
        ? state->random.cone(emitter.direction, emitter.spread)
        : glm::normalize(emitter.direction);
    const float speed = state->random.range(emitter.speedMin, emitter.speedMax);
    const float lifetime = state->random.range(emitter.lifeMin, emitter.lifeMax);
    const float phase = state->random.unit();

    Particle particle;
    particle.position = position;
    particle.previous = position;
    particle.velocity = orientation * (heading * speed);
    particle.age = 0.0f;
    particle.lifetime = lifetime;
    particle.phase = phase;
    state->particles.push_back(particle);
    return true;
}

void step(const Emitter& emitter, State* state, const glm::vec3& origin, const glm::quat& orientation, float seconds) {
    travel(emitter, state, seconds);
    burst(emitter, state, origin, orientation, owing(state, emitter.rate, seconds));
}

void burst(const Emitter& emitter, State* state, const glm::vec3& origin, const glm::quat& orientation, uint32_t count) {
    for (uint32_t spawned = 0; spawned < count && state->particles.size() < emitter.cap; spawned++) {
        spawn(emitter, state, origin + (orientation * birthplace(emitter, &state->random)), orientation);
    }
}

};  // namespace v3d::type::effect
