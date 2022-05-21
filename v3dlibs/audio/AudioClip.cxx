/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "AudioClip.h"

#include <iostream>

#ifdef WIN32
#include <al.h>
#include <alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#include <AL/alut.h>

#include <log4cxx/logger.h>

using namespace v3d::audio;

AudioClip::AudioClip()
{
}

AudioClip::~AudioClip()
{
}

unsigned int AudioClip::source() const
{
	return source_;
}

void AudioClip::destroy()
{
	alDeleteBuffers(1, &buffer_);
	alDeleteSources(1, &source_);
}

bool AudioClip::load(const std::string & filename)
{
	ALboolean loop = false;
	log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger("v3d.audio"));

	// load sound data from file
	buffer_ = alutCreateBufferFromFile(filename.c_str());
	if (alGetError() != AL_NO_ERROR)
	{
		LOG4CXX_ERROR(logger, "alut create failed for file [" << filename << "]");
		return false;
	}

	// create a source for the sound
	alGenSources(1, &source_);
	if (alGetError() != AL_NO_ERROR)
	{
		LOG4CXX_ERROR(logger, "error generating OpenAL sources for audio clip [" << filename << "]");
		return false;
	}

	// link the sound buffer to the source
	alSourcei(source_, AL_BUFFER, buffer_);
	// and set the source properties
	alSourcef(source_, AL_PITCH, 1.0f);
	alSourcef(source_, AL_GAIN, 1.0f);
	/* these next two set an error bit...
	alSourcefv(_source, AL_POSITION, _position.raw());
	alSourcefv(_source, AL_VELOCITY, _velocity.raw());
	*/
	alSourcei(source_, AL_LOOPING, loop);

	if (alGetError() != AL_NO_ERROR)
	{
		LOG4CXX_ERROR(logger, "error setting source properties for audio clip [" << filename << "]");
		return false;
	}

	return true;
}

