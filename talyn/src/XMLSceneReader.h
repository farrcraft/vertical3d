/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "RenderContext.h"

#include <string>

namespace Talyn
{

	class XMLSceneReader
	{
		public:
			bool read(const boost::shared_ptr<RenderContext> & rc, const std::string & filename);

		protected:
	};

}; // end namespace Talyn
