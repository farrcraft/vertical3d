/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../3dtypes/Vector3.h"

#include <string>

namespace v3d::audio
{

	class AudioClip
	{
		public:
			AudioClip();
			~AudioClip();

			bool load(const std::string & filename);
			void destroy();

	//		unsigned int buffer(void) const;
			unsigned int source() const;

	//		vector3d position(void) const;
	//		vector3d velocity(void) const;

		private:
			unsigned int buffer_;
			unsigned int source_;

			v3d::types::Vector3 position_;
			v3d::types::Vector3 velocity_;
	};

};
