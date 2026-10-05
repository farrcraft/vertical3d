/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Sound.h"

#include <string>

namespace v3d::audio::kind {
/**
 **/
Sound::Sound(const std::string& name, v3d::asset::Type t, const boost::shared_ptr<AudioClip>& clip) :
    v3d::asset::Asset(name, t),
    clip_(clip) {
}

/**
 **/
boost::shared_ptr<AudioClip> Sound::clip() {
    return clip_;
}

};  // namespace v3d::audio::kind
