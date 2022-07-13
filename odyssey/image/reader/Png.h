/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../Reader.h"

namespace odyssey::image::reader {

	class Png : public odyssey::image::Reader {
		public:
			Png();
			~Png();

			virtual boost::shared_ptr<Image> read(std::string_view filename);
	};

}; // end namespace
