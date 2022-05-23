/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Image.h"

#include <boost/shared_ptr.hpp>

#include <string>

namespace v3d::image {
	/**
	 *  
	 **/
	class ImageWriter {
		public:
			ImageWriter();
			virtual ~ImageWriter();

			virtual bool write(const std::string & filename, const boost::shared_ptr<Image> & img);

		private:
	};

}; 
