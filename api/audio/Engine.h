/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <map>
#include <string>

#include "AudioClip.h"
#include "../asset/Json.h"
#include "../log/Logger.h"

#include <soloud.h>
#include <boost/shared_ptr.hpp>

namespace v3d::audio {

    /**
     * An Audio / Sound processing engine
     **/
    class Engine final {
     public:
        Engine(const boost::shared_ptr<v3d::log::Logger> & logger);
        ~Engine() = default;

        bool initialize();
        void shutdown();
        bool load(const boost::shared_ptr<v3d::asset::Json> & config);
        bool loadClip(const std::string_view & filename, const std::string_view & key);
        bool playClip(const std::string_view & clip);

     private:
        boost::shared_ptr<v3d::log::Logger> logger_;
        SoLoud::Soloud soloud_;
        std::map<std::string, boost::shared_ptr<AudioClip>> sounds_;
    };

};  // namespace v3d::audio
