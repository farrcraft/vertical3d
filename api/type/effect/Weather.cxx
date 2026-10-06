/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Weather.h"

#include "Particle.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <glm/gtc/quaternion.hpp>

namespace v3d::type::effect {

namespace {

/**
 * How far a coordinate has to move to come back into [low, high), whole widths at a time.
 **/
float wrapping(float value, float low, float high) {
    const float width = high - low;
    if (!(width > 0.0f)) {
        return 0.0f;
    }
    return -std::floor((value - low) / width) * width;
}

};  // namespace

void fall(const Emitter& emitter, Weather* weather, State* state, const glm::vec3& minimum,
    const glm::vec3& maximum, float seconds) {
    // an ease below nothing moves nothing, and the target is held between 0 and 1, so the
    // intensity only ever eases towards a value in that range
    const float easing = std::max(0.0f, weather->ease * seconds);
    // a target that is not finite is no target, and the intensity stays where it is
    const float target = std::isfinite(weather->target) ? std::clamp(weather->target, 0.0f, 1.0f) : weather->intensity;
    weather->intensity += std::clamp(target - weather->intensity, -easing, easing);

    travel(emitter, state, seconds, weather->wind);
    std::vector<Particle>& particles = state->particles;
    std::size_t index = 0;
    while (index < particles.size()) {
        Particle& particle = particles[index];
        if (particle.position.y < minimum.y) {
            particle = particles.back();
            particles.pop_back();
            continue;
        }
        const glm::vec3 shift(wrapping(particle.position.x, minimum.x, maximum.x), 0.0f,
            wrapping(particle.position.z, minimum.z, maximum.z));
        particle.position += shift;
        particle.previous += shift;
        index++;
    }

    const float area = std::max(0.0f, maximum.x - minimum.x) * std::max(0.0f, maximum.z - minimum.z);
    const uint32_t due = owing(state, weather->density * area * weather->intensity, seconds);
    const glm::quat unturned = glm::identity<glm::quat>();
    for (uint32_t spawned = 0; spawned < due && particles.size() < emitter.cap; spawned++) {
        const float x = state->random.range(minimum.x, maximum.x);
        const float z = state->random.range(minimum.z, maximum.z);
        spawn(emitter, state, glm::vec3(x, maximum.y, z), unturned);
    }
}

};  // namespace v3d::type::effect
