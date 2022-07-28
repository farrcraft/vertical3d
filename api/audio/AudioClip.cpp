/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "AudioClip.h"

#include <iostream>
#include <boost/make_shared.hpp>

namespace v3d::audio {
    void AudioClip::destroy() {
    }

    bool AudioClip::load(const std::string_view & filename) {
        const std::string str(filename);
        wav_ = boost::make_shared<SoLoud::Wav>();
        wav_->load(str.c_str());
        return true;
    }

    boost::shared_ptr<SoLoud::Wav> AudioClip::wav() {
        return wav_;
    }

};  // namespace v3d::audio
