/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../ImageWriter.h"

namespace v3d::image::writer {
	class JPEGWriter final : public ImageWriter {
		public:
			JPEGWriter();
			~JPEGWriter();

			virtual bool write(const std::string & filename, const boost::shared_ptr<Image> & img);
	};

};
