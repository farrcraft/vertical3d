/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "SoundEngine.h"

#include <iostream>

#ifdef WIN32
#include <al.h>
#include <alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#include <AL/alut.h>

#include <boost/foreach.hpp>

using namespace v3d::audio;

SoundEngine::SoundEngine() {
	// initialize OpenAL
	alutInit(0, 0);
	// clear any leftover error bits
	alGetError();

	// set listener properties
	updateListener();
}

void SoundEngine::shutdown() {
	for (std::map<std::string, AudioClip>::iterator i = sounds_.begin(); i != sounds_.end(); i++) {
		(*i).second.destroy();
	}
	alutExit();
}

bool SoundEngine::load(const boost::property_tree::ptree & tree, const std::string & assetPath) {
	std::string key, filename, wav;
	// log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger("v3d.audio"));
	// LOG4CXX_DEBUG(logger, "SoundEngine::load - looking for sound clips to load...");
	BOOST_FOREACH(boost::property_tree::ptree::value_type const & v, tree.get_child("config.sounds")) {
		if (v.first == "clip") {
			key = v.second.get<std::string>("<xmlattr>.id");
			filename = v.second.get<std::string>("<xmlattr>.file");
			wav = assetPath + filename;
			loadClip(wav, key);
		}
	}
	return true;
}

bool SoundEngine::loadClip(const std::string & filename, const std::string & key) {
	// log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger("v3d.audio"));
	// LOG4CXX_DEBUG(logger, "SoundEngine::loadClip - loading audio clip with filename [" << filename << "] with id [" << key << "]");

	AudioClip clip;

	if (!clip.load(filename)) {
		return false;
	}
	sounds_[key] = clip;

	return true;
}

bool SoundEngine::playClip(const std::string & clip) {
	if (sounds_.find(clip) == sounds_.end()) {
		return false;
	}

	alSourcePlay(sounds_[clip].source());
	return true;
}

void SoundEngine::updateListener() {
	alListenerfv(AL_POSITION, *listenerPosition_);
	alListenerfv(AL_VELOCITY, *listenerVelocity_);

	ALfloat listenerOrientation[] = {0.0, 0.0, -1.0, 0.0, 1.0, 0.0 };
	alListenerfv(AL_ORIENTATION, listenerOrientation);
}
