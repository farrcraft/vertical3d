/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "AudioClip.h"

#include <boost/property_tree/ptree.hpp>
#include <map>

namespace v3d::audio {

	/**
	 * The SoundEngine class. It is responsible for all of the OpenAL functionality. 
	 **/
	class SoundEngine final {
		public:
			SoundEngine();
			~SoundEngine() = default;

			void shutdown();
			bool load(const boost::property_tree::ptree & tree, const std::string & assetPath);
			bool loadClip(const std::string & filename, const std::string & key);
			bool playClip(const std::string & clip);
			void updateListener();

		private:
			std::map<std::string, AudioClip> sounds_;

			v3d::type::Vector3 listenerPosition_;
			v3d::type::Vector3 listenerVelocity_;
	};

};
