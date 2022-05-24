/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "StyleProperty.h"

#include <string>

namespace Luxa
{

	/**
	 * A style property that defines a font.
	 */
	class FontStyleProperty : public StyleProperty
	{
		public:
			FontStyleProperty(const std::string & name, const std::string & src);
			~FontStyleProperty();

			bool italics() const;
			bool bold() const;
			std::string face() const;
			unsigned int size() const;
			std::string source() const;

		private:
			std::string source_;
			bool italics_;
			bool bold_;
			std::string face_;
			unsigned int size_;
	};

}; // end namespace Luxa
