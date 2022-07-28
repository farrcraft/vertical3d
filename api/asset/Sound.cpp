/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Sound.h"

namespace v3d::asset {
    /**
     **/
    Sound::Sound(std::string_view name, Type t, boost::shared_ptr<v3d::audio::AudioClip> clip) :
        Asset(name, t),
        clip_(clip) {
    }

    /**
     **/
    boost::shared_ptr<v3d::audio::AudioClip> Sound::clip() {
        return clip_;
    }

};  // namespace v3d::asset
