/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <soloud_wav.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::audio {

    class AudioClip final {
     public:
        AudioClip() = default;
        ~AudioClip() = default;

        bool load(const std::string_view & filename);
        void destroy();

        boost::shared_ptr<SoLoud::Wav> wav();

     private:
        boost::shared_ptr<SoLoud::Wav> wav_;
    };

};  // namespace v3d::audio
