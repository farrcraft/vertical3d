/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Asset.h"
#include "../image/Image.h"

#include <boost/shared_ptr.hpp>

namespace odyssey::asset {
	/**
	 **/
	class Image : public Asset {
	public:
		/**
		 **/
		Image(std::string_view name, Type t, boost::shared_ptr<odyssey::image::Image> img);

		/**
		 **/
		boost::shared_ptr<odyssey::image::Image> image();

	private:
		boost::shared_ptr<odyssey::image::Image> image_;
	};
};
