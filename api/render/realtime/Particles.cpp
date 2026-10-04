/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Particles.h"

#include <api/render/realtime/component/Particles.h>

#include <api/ecs/component/Emitter.h>
#include <api/type/effect/Emitter.h>

#include <algorithm>
#include <cmath>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime {

namespace {

/**
 * The region a particle shows at its age.
 **/
void region(const component::Particles& look, const type::effect::Particle& particle, glm::vec2* uv0, glm::vec2* uv1) {
    if (!look.clip) {
        *uv0 = look.uv0;
        *uv1 = look.uv1;
        return;
    }
    const float time = look.overLife ? particle.life() * look.clip->clock().duration() : particle.age;
    const type::animation::SpriteClip::Frame& frame = look.clip->frame(time);
    *uv0 = frame.uv0;
    *uv1 = frame.uv1;
}

/**
 * The four corners of a particle's quad around its centre, top-left first so that uv0 lands
 * there. Along is the quad's length and side its width, both unit vectors in the camera's plane.
 **/
WorldCanvas::Corners corners(const glm::vec3& centre, const glm::vec3& along, float length,
    const glm::vec3& side, float width) {
    const glm::vec3 head = centre + (along * (length * 0.5f));
    const glm::vec3 tail = centre - (along * (length * 0.5f));
    const glm::vec3 half = side * (width * 0.5f);
    return { head - half, head + half, tail + half, tail - half };
}

};  // namespace

void particles(const entt::registry& registry, float alpha, const glm::vec3& right,
    const glm::vec3& up, const glm::vec3& depthAxis, DepthOrder* order) {
    auto view = registry.view<const ecs::component::Emitter, const component::Particles>();
    for (const entt::entity entity : view) {
        const type::effect::Emitter& description = view.get<const ecs::component::Emitter>(entity).description;
        const component::Particles& look = view.get<const component::Particles>(entity);
        for (const type::effect::Particle& particle : view.get<const ecs::component::Emitter>(entity).state.particles) {
            glm::vec3 centre = glm::mix(particle.previous, particle.position, alpha);
            if (description.sway != 0.0f) {
                const float turns = particle.phase + (particle.age * description.swayRate);
                centre += right * (description.sway * std::sin(turns * glm::two_pi<float>()));
            }
            const float life = particle.life();
            const float size = description.size.sample(life);
            const glm::vec4 colour = description.colour.sample(life);
            glm::vec2 uv0;
            glm::vec2 uv1;
            region(look, particle, &uv0, &uv1);

            glm::vec3 along = up;
            glm::vec3 side = right;
            float length = size;
            if (look.facing == component::Particles::Facing::Velocity) {
                // the velocity as the camera sees it, in the right and up basis
                const glm::vec2 seen(glm::dot(particle.velocity, right), glm::dot(particle.velocity, up));
                const float speed = glm::length(seen);
                if (speed > 0.0f) {
                    const glm::vec2 heading = seen / speed;
                    along = (right * heading.x) + (up * heading.y);
                    side = (right * heading.y) - (up * heading.x);
                    length = std::max(size, speed * look.stretch);
                }
            }
            order->quad(glm::dot(centre, depthAxis), corners(centre, along, length, side, size), uv0, uv1, colour, look.texture);
        }
    }
}

};  // namespace v3d::render::realtime
