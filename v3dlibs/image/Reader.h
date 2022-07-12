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
	class Reader {
		public:
			Reader() = default;
			virtual ~Reader() = default;

			virtual boost::shared_ptr<Image> read(std::string_view filename);
	};

};
