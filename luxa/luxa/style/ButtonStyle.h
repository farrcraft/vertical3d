/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Style.h"
#include "../Button.h"

namespace Luxa
{

	/**
	 * A style for buttons.
	 */
	class ButtonStyle : public Style
	{
		public:
			ButtonStyle(const std::string & str, Button::ButtonState s);
			~ButtonStyle();

			/**
			 * Get the button state this style is used for.
			 * @return the button state
			 */
			Button::ButtonState state() const;

		private:
			Button::ButtonState state_;
	};

}; // end namespace Luxa
