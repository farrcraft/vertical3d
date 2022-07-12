/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Reader.h"

namespace v3d::image::reader
{

	class Bmp final : public v3d::image::Reader {
		public:
			Bmp() = default;
			~Bmp() = default;

			virtual boost::shared_ptr<Image> read(std::string_view filename);
	};

};
