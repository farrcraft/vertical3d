/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "FmodEvents.h"

#include <fmod_errors.h>
#include <SDL3/SDL_loadso.h>

#include <stdexcept>
#include <string>
#include <string_view>

#include <fmod_studio.hpp>

namespace v3d::audio {

namespace {

/**
 * How many event instances FMOD may have playing at once, across every bank.
 **/
const int maximumChannels = 512;

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
bool FmodEvents::play(std::string_view event) {
    const std::string path(event);
    FMOD::Studio::EventDescription* description = nullptr;
    FMOD_RESULT result = system_->getEvent(path.c_str(), &description);
    if (result != FMOD_OK) {
        logger_->get()->warn("FMOD has no event {}: {}", path, FMOD_ErrorString(result));
        return false;
    }
    FMOD::Studio::EventInstance* instance = nullptr;
    result = description->createInstance(&instance);
    if (result != FMOD_OK) {
        logger_->get()->warn("FMOD could not create an instance of {}: {}", path, FMOD_ErrorString(result));
        return false;
    }
    result = instance->start();
    // released straight away: FMOD frees the instance once it has finished playing
    instance->release();
    return result == FMOD_OK;
}

/**
 **/
bool FmodEvents::parameter(std::string_view name, float value) {
    const std::string parameter(name);
    const FMOD_RESULT result = system_->setParameterByName(parameter.c_str(), value);
    if (result != FMOD_OK) {
        logger_->get()->warn("FMOD could not set the global parameter {}: {}", parameter, FMOD_ErrorString(result));
        return false;
    }
    return true;
}

/**
 **/
void FmodEvents::update() {
    system_->update();
}

/**
 **/
std::string_view FmodEvents::name() const noexcept {
    return "fmod";
}

};  // namespace v3d::audio
