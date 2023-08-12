/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Style.h"
#include "../component/Button.h"

namespace v3d::ui::style {

	/**
	 * A style for buttons.
	 */
	class Button : public Style
	{
	public:
		Button(const std::string& str, v3d::ui::component::Button::ButtonState s);
		~Button();

		/**
		 * Get the button state this style is used for.
		 * @return the button state
		 */
		v3d::ui::component::Button::ButtonState state() const;

	private:
		v3d::ui::component::Button::ButtonState state_;
	};

}; // end namespace v3d::ui::style
