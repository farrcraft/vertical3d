/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../ImageReader.h"

namespace v3d::image::reader
{

	class BMPReader final : public ImageReader {
		public:
			BMPReader();
			~BMPReader();

			virtual boost::shared_ptr<Image> read(const std::string & filename);
	};

};
