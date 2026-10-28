/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <optional>

#include <glm/vec3.hpp>

namespace v3d::audio {

/**
 * Where sound is heard from, in the game's world coordinates: a camera's or a player's eye and
 * its three axes. Right is given, not worked out from forward and up. A camera may be built in
 * either hand, and right decides which speaker a sound is heard from.
 * A camera::Profile's eye(), direction(), up() and right() are a listener's position, forward,
 * up and right.
 *
 * The default listener stands at the origin facing -z, with +y up and +x to its right, where the
 * world and the mixer's axes agree.
 **/
struct Listener final {
    glm::vec3 position{0.0F, 0.0F, 0.0F};
    glm::vec3 forward{0.0F, 0.0F, -1.0F};  /**< the way the listener faces **/
    glm::vec3 up{0.0F, 1.0F, 0.0F};        /**< the top of the listener's head **/
    glm::vec3 right{1.0F, 0.0F, 0.0F};     /**< the listener's right hand, as on the screen **/
};

/**
 * A listener whose forward, up and right are unit length and at right angles. Forward keeps
 * its direction, up is turned towards it within the plane they share, and right is the side of
 * that plane the given right is on.
 *
 * @return nothing for a value that is not finite, or a forward or an up of zero length. The
 *         same holds for a forward and an up that are parallel, and for a right that lies in
 *         the plane of the other two. None of those says which way the listener faces or which
 *         side is its right
 **/
std::optional<Listener> orthonormal(const Listener& listener) noexcept;

/**
 * @return whether a listener orthonormal() returned has its right at up x forward. A camera of
 *         the other hand to glm::lookAt builds that basis
 **/
bool mirrored(const Listener& listener) noexcept;

/**
 * Where a point in the world is, as the listener hears it: x to the right, y up and z behind,
 * which are the axes SDL_mixer places a track in.
 *
 * @param listener a listener orthonormal() returned
 **/
glm::vec3 relative(const Listener& listener, const glm::vec3& point) noexcept;

};  // namespace v3d::audio
