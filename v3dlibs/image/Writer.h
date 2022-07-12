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
	class Writer {
		public:
			Writer() = default;
			virtual ~Writer() = default;

			virtual bool write(std::string_view filename, const boost::shared_ptr<Image> & img);
	};

}; 
