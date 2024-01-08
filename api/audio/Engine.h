/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

// Fix necessary for EnTT - https://github.com/skypjack/entt/issues/96#issuecomment-395867237
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <soloud.h>

#include <map>
#include <string>

#include "AudioClip.h"
#include "../asset/Json.h"
#include "../log/Logger.h"
#include "../event/Sound.h"

#include <boost/shared_ptr.hpp>

#include <entt/entt.hpp>

namespace v3d::audio {

    /**
     * An Audio / Sound processing engine
     **/
    class Engine final {
     public:
        Engine(const boost::shared_ptr<v3d::log::Logger> & logger, const boost::shared_ptr<entt::dispatcher>& dispatcher);
        ~Engine() = default;

        bool initialize();
        void shutdown();
        bool load(const boost::shared_ptr<v3d::asset::Json> & config);
        bool loadClip(const std::string_view & filename, const std::string_view & key);
        bool playClip(const std::string_view & clip);
        void soundEvent(const v3d::event::Sound & sound);

     private:
        boost::shared_ptr<entt::dispatcher> dispatcher_;
        boost::shared_ptr<v3d::log::Logger> logger_;
        SoLoud::Soloud soloud_;
        std::map<std::string, boost::shared_ptr<AudioClip>> sounds_;
    };

};  // namespace v3d::audio
