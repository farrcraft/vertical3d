/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <glm/vec3.hpp>

namespace v3d::event::kind {

/**
 * A request to play a clip once. audio::Engine listens for it.
 *
 * A sound is either panned or placed. One made with a pan sits between the speakers, as
 * audio::Play::pan places it. One made with a position is heard from the audio engine's
 * listener, as audio::Play::position places it.
 **/
class Sound final {
 public:
    /**
     * @param clip the id the clip was filed under
     * @param pan where the sound sits between the speakers, from -1 left to 1 right
     **/
    // not noexcept: clip_ is a std::string built from the view, which allocates
    Sound(const std::string_view &clip, float pan = 0.0F);
    /**
     * @param clip the id the clip was filed under
     * @param position where the sound is in the world, in the coordinates the listener is in
     **/
    Sound(const std::string_view &clip, const glm::vec3& position);
    std::string_view clip() const noexcept;
    /**
     * @return the pan, which is 0 for a placed sound
     **/
    float pan() const noexcept;
    /**
     * @return where the sound is, or nothing for a panned sound
     **/
    std::optional<glm::vec3> position() const noexcept;

 private:
    std::string clip_;
    float pan_;
    std::optional<glm::vec3> position_;
};
};  // namespace v3d::event::kind
