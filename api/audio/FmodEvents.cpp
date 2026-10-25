/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "FmodEvents.h"

#include <api/log/Logger.h>

#include <fmod_common.h>
#include <fmod_errors.h>
#include <fmod_studio_common.h>
#include <SDL3/SDL_loadso.h>

#include <algorithm>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Events.h"

#include <fmod_studio.hpp>

namespace v3d::audio {

namespace {

/**
 * How many event instances FMOD may have playing at once, across every bank.
 **/
const int maximumChannels = 512;

/**
 * Whether a mixer path names a VCA rather than a bus.
 **/
bool isVca(std::string_view path) noexcept {
    return path.starts_with("vca:/");
}

};  // namespace

/**
 **/
FmodEvents::FmodEvents(const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger),
    system_(nullptr) {
    // The build delay-loads fmodstudio.dll, and a delay-loaded call into a dll that cannot be
    // found ends the process. Loading it here first turns a missing dll into an exception.
    SDL_SharedObject* studio = SDL_LoadObject("fmodstudio.dll");
    if (studio == nullptr) {
        throw std::runtime_error(std::string("FMOD's dlls could not be loaded: ") + SDL_GetError());
    }
    SDL_UnloadObject(studio);
    FMOD_RESULT result = FMOD::Studio::System::create(&system_);
    if (result != FMOD_OK) {
        throw std::runtime_error(std::string("FMOD could not create a Studio system: ") + FMOD_ErrorString(result));
    }
    result = system_->initialize(maximumChannels, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK) {
        system_->release();
        system_ = nullptr;
        throw std::runtime_error(std::string("FMOD could not initialize its Studio system: ") + FMOD_ErrorString(result));
    }
}

/**
 **/
FmodEvents::~FmodEvents() {
    if (system_ != nullptr) {
        // releases the banks and every event instance with the system
        system_->release();
    }
}

/**
 **/
bool FmodEvents::bank(const std::string& path) {
    FMOD::Studio::Bank* loaded = nullptr;
    const FMOD_RESULT result = system_->loadBankFile(path.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &loaded);
    if (result != FMOD_OK) {
        logger_->get()->error("FMOD could not load the bank {}: {}", path, FMOD_ErrorString(result));
        return false;
    }
    return true;
}

/**
 **/
FMOD::Studio::EventInstance* FmodEvents::create(const std::string& path) {
    FMOD::Studio::EventDescription* description = nullptr;
    FMOD_RESULT result = system_->getEvent(path.c_str(), &description);
    if (result != FMOD_OK) {
        if (missingEvents_.insert(path).second) {
            logger_->get()->warn("FMOD has no event {}: {}", path, FMOD_ErrorString(result));
        }
        return nullptr;
    }
    FMOD::Studio::EventInstance* instance = nullptr;
    result = description->createInstance(&instance);
    if (result != FMOD_OK) {
        logger_->get()->warn("FMOD could not create an instance of {}: {}", path, FMOD_ErrorString(result));
        return nullptr;
    }
    return instance;
}

/**
 **/
bool FmodEvents::play(std::string_view event) {
    FMOD::Studio::EventInstance* instance = create(std::string(event));
    if (instance == nullptr) {
        return false;
    }
    const FMOD_RESULT result = instance->start();
    // released straight away: FMOD frees the instance once it has finished playing
    instance->release();
    return result == FMOD_OK;
}

/**
 **/
Instance FmodEvents::start(std::string_view event) {
    FMOD::Studio::EventInstance* instance = create(std::string(event));
    if (instance == nullptr) {
        return Instance::None;
    }
    if (instance->start() != FMOD_OK) {
        instance->release();
        return Instance::None;
    }
    const Instance started = static_cast<Instance>(nextInstance_++);
    started_[started] = instance;
    return started;
}

/**
 **/
bool FmodEvents::stop(Instance instance, Stop how) {
    if (!playing(instance)) {
        return false;
    }
    // kept until update() sees it stopped, so playing() reads FMOD's state through a fade out
    const FMOD_STUDIO_STOP_MODE mode = how == Stop::FadeOut ? FMOD_STUDIO_STOP_ALLOWFADEOUT : FMOD_STUDIO_STOP_IMMEDIATE;
    return started_.at(instance)->stop(mode) == FMOD_OK;
}

/**
 **/
bool FmodEvents::playing(Instance instance) const {
    const std::map<Instance, FMOD::Studio::EventInstance*>::const_iterator found = started_.find(instance);
    if (found == started_.end()) {
        return false;
    }
    FMOD_STUDIO_PLAYBACK_STATE state = FMOD_STUDIO_PLAYBACK_STOPPED;
    return found->second->getPlaybackState(&state) == FMOD_OK && state != FMOD_STUDIO_PLAYBACK_STOPPED;
}

/**
 **/
bool FmodEvents::parameter(std::string_view name, float value) {
    const std::string parameter(name);
    const FMOD_RESULT result = system_->setParameterByName(parameter.c_str(), value);
    if (result != FMOD_OK) {
        if (refusedParameters_.insert(parameter).second) {
            logger_->get()->warn("FMOD could not set the global parameter {}: {}", parameter, FMOD_ErrorString(result));
        }
        return false;
    }
    return true;
}

/**
 **/
void FmodEvents::refuse(const std::string& path, const char* why) {
    if (refusedMixes_.insert(path).second) {
        logger_->get()->warn("FMOD refused the mixer path {}: {}", path, why);
    }
}

/**
 **/
bool FmodEvents::volume(std::string_view path, float level) {
    const std::string mix(path);
    const float clamped = std::max(level, 0.0F);
    FMOD_RESULT result = FMOD_OK;
    if (isVca(path)) {
        FMOD::Studio::VCA* vca = nullptr;
        result = system_->getVCA(mix.c_str(), &vca);
        if (result == FMOD_OK) {
            result = vca->setVolume(clamped);
        }
    } else {
        FMOD::Studio::Bus* bus = nullptr;
        result = system_->getBus(mix.c_str(), &bus);
        if (result == FMOD_OK) {
            result = bus->setVolume(clamped);
        }
    }
    if (result != FMOD_OK) {
        refuse(mix, FMOD_ErrorString(result));
        return false;
    }
    return true;
}

/**
 **/
std::optional<float> FmodEvents::volume(std::string_view path) const {
    const std::string mix(path);
    float level = 0.0F;
    FMOD_RESULT result = FMOD_OK;
    if (isVca(path)) {
        FMOD::Studio::VCA* vca = nullptr;
        result = system_->getVCA(mix.c_str(), &vca);
        if (result == FMOD_OK) {
            result = vca->getVolume(&level);
        }
    } else {
        FMOD::Studio::Bus* bus = nullptr;
        result = system_->getBus(mix.c_str(), &bus);
        if (result == FMOD_OK) {
            result = bus->getVolume(&level);
        }
    }
    if (result != FMOD_OK) {
        return std::nullopt;
    }
    return level;
}

/**
 **/
bool FmodEvents::paused(std::string_view bus, bool paused) {
    const std::string mix(bus);
    if (isVca(bus)) {
        refuse(mix, "a VCA has no pause");
        return false;
    }
    FMOD::Studio::Bus* found = nullptr;
    FMOD_RESULT result = system_->getBus(mix.c_str(), &found);
    if (result == FMOD_OK) {
        result = found->setPaused(paused);
    }
    if (result != FMOD_OK) {
        refuse(mix, FMOD_ErrorString(result));
        return false;
    }
    return true;
}

/**
 **/
std::optional<bool> FmodEvents::paused(std::string_view bus) const {
    if (isVca(bus)) {
        return std::nullopt;
    }
    const std::string mix(bus);
    FMOD::Studio::Bus* found = nullptr;
    bool paused = false;
    if (system_->getBus(mix.c_str(), &found) != FMOD_OK || found->getPaused(&paused) != FMOD_OK) {
        return std::nullopt;
    }
    return paused;
}

/**
 **/
void FmodEvents::update() {
    system_->update();
    for (std::map<Instance, FMOD::Studio::EventInstance*>::iterator started = started_.begin(); started != started_.end();) {
        if (playing(started->first)) {
            ++started;
            continue;
        }
        started->second->release();
        started = started_.erase(started);
    }
}

/**
 **/
std::string_view FmodEvents::name() const noexcept {
    return "fmod";
}

};  // namespace v3d::audio
