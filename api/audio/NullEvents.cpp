/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "NullEvents.h"

#include <optional>
#include <string>
#include <string_view>

#include "Events.h"
#include "Listener.h"

#include <glm/vec3.hpp>

namespace v3d::audio {

/**
 **/
bool NullEvents::bank(const std::string& /* path */) {
    return false;
}

/**
 **/
bool NullEvents::play(std::string_view /* event */) {
    return false;
}

/**
 **/
Instance NullEvents::start(std::string_view /* event */) {
    return Instance::None;
}

/**
 **/
bool NullEvents::play(std::string_view /* event */, const glm::vec3& /* position */) {
    return false;
}

/**
 **/
Instance NullEvents::start(std::string_view /* event */, const glm::vec3& /* position */) {
    return Instance::None;
}

/**
 **/
bool NullEvents::position(Instance /* instance */, const glm::vec3& /* position */) {
    return false;
}

/**
 **/
std::optional<glm::vec3> NullEvents::position(Instance /* instance */) const {
    return std::nullopt;
}

/**
 **/
bool NullEvents::listener(const Listener& /* listener */) {
    return false;
}

/**
 **/
bool NullEvents::stop(Instance /* instance */, Stop /* how */) {
    return false;
}

/**
 **/
bool NullEvents::playing(Instance /* instance */) const {
    return false;
}

/**
 **/
bool NullEvents::parameter(std::string_view /* name */, float /* value */) {
    return false;
}

/**
 **/
bool NullEvents::volume(std::string_view /* path */, float /* level */) {
    return false;
}

/**
 **/
std::optional<float> NullEvents::volume(std::string_view /* path */) const {
    return std::nullopt;
}

/**
 **/
bool NullEvents::paused(std::string_view /* bus */, bool /* paused */) {
    return false;
}

/**
 **/
std::optional<bool> NullEvents::paused(std::string_view /* bus */) const {
    return std::nullopt;
}

/**
 **/
void NullEvents::update() {
}

/**
 **/
std::string_view NullEvents::name() const noexcept {
    return "null";
}

};  // namespace v3d::audio
