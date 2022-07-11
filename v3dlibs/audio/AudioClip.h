/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/glm.hpp>

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

			glm::vec3 position_;
			glm::vec3 velocity_;
	};

};
