/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Image.h"

#include <boost/shared_ptr.hpp>

namespace odyssey::image {

	class Reader {
	public:
		Reader();
		virtual ~Reader();

		virtual boost::shared_ptr<Image> read(std::string_view filename);

	private:

	};

}; // end namespace

