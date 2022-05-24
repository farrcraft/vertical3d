/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../ImageReader.h"

namespace v3d::image::reader
{

	class TGAReader : public ImageReader
	{
		public:
			TGAReader();
			~TGAReader();

			virtual boost::shared_ptr<Image> read(const std::string & filename);
	};

};
