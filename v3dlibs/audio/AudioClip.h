/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../type/Vector3.h"

#include <string>

namespace v3d::audio {

	class AudioClip final {
		public:
			AudioClip() = default;
			~AudioClip() = default;

			bool load(const std::string & filename);
			void destroy();

	//		unsigned int buffer(void) const;
			unsigned int source() const noexcept;

	//		vector3d position(void) const;
	//		vector3d velocity(void) const;

		private:
			unsigned int buffer_;
			unsigned int source_;

			v3d::type::Vector3 position_;
			v3d::type::Vector3 velocity_;
	};

};
