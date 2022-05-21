/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#ifndef INCLUDED_LUXA_BUTTONSTYLE
#define INCLUDED_LUXA_BUTTONSTYLE

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


#endif // INCLUDED_LUXA_BUTTONSTYLE
