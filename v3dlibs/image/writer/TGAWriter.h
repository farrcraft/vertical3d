/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../ImageWriter.h"

namespace v3d::image::writer {
	class TGAWriter final : public ImageWriter {
		public:
			TGAWriter();
			~TGAWriter();

			virtual bool write(const std::string & filename, const boost::shared_ptr<Image> & img);
	};

};