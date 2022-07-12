/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Writer.h"

namespace v3d::image::writer {
	class Jpeg final : public v3d::image::Writer {
		public:
			Jpeg() = default;
			~Jpeg() = default;

			virtual bool write(std::string_view filename, const boost::shared_ptr<Image> & img);
	};

};
