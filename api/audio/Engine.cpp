/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Engine.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

namespace v3d::audio {

    Engine::Engine(const boost::shared_ptr<v3d::log::Logger> & logger) : logger_(logger) {
    }

    void Engine::shutdown() {
        for (auto it = sounds_.begin(); it != sounds_.end(); it++) {
            it->second->destroy();
        }
        soloud_.deinit();
    }

    bool Engine::initialize() {
        soloud_.init();

        return true;
    }

    bool Engine::load(const boost::shared_ptr<v3d::asset::Json>& config) {
        auto const doc = config->document();
        auto const sounds = doc.at("sounds");
        if (!sounds.is_array()) {
            LOG_ERROR(logger_) << "Missing sounds in audio config";
            return false;
        }
        // for each sound
        auto const items = sounds.as_array();
        auto it = items.begin();
        for (; it != items.end(); ++it) {
            if (!it->is_object()) {
                LOG_ERROR(logger_) << "Unrecognized sound config";
                return false;
            }
            auto const sound = it->as_object();
            const std::string clipId = boost::json::value_to<std::string>(sound.at("clip_id"));
            const std::string fileName = boost::json::value_to<std::string>(sound.at("file"));
            loadClip(fileName, clipId);
        }

        return true;
    }

    bool Engine::loadClip(const std::string_view & filename, const std::string_view & key) {
        LOG_DEBUG(logger_) << "SoundEngine::loadClip - loading audio clip with filename [" << filename << "] with id [" << key << "]";

        boost::shared_ptr<AudioClip> clip = boost::make_shared<AudioClip>();

        // Might want to use the asset loader here instead of loading directly
        if (!clip->load(filename)) {
            return false;
        }
        const std::string clipId(key);
        sounds_[clipId] = clip;

        return true;
    }

    bool Engine::playClip(const std::string_view & clip) {
        const std::string clipId(clip);
        if (sounds_.find(clipId) == sounds_.end()) {
            return false;
        }
        soloud_.play(*sounds_[clipId]->wav());
        return true;
    }

};  // namespace v3d::audio
