/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once


#include "../Reader.h"

namespace odyssey::image::reader {

	class Jpeg : public odyssey::image::Reader
	{
	public:
		Jpeg();
		~Jpeg();

		virtual boost::shared_ptr<Image> read(std::string_view filename);
	};

}; // end namespace

