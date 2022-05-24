/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Component.h"

#include <string>

namespace Luxa
{
	class ComponentManager;

	/**
	 * A vGUI Button
	 */
	class Button : public Component
	{
		public:
			Button(ComponentManager * cm);

			/**
			 * button state enumeration
			 */
			typedef enum 
			{
				STATE_NORMAL,	/**< Normal Button State **/
				STATE_HOVER,	/**< Mouse is hovering over button **/
				STATE_PRESS,	/**< Button is being clicked **/
				STATE_INACTIVE	/**< unclickable/insensitive state **/
			} ButtonState;

			/**
			 * Set the button's label
			 * @param str the new label text
			 */
			void label(const std::string & str);
			/**
			 * Get the button's label
			 * @return the current label text
			 */
			std::string label() const;
			/**
			 * Get the current button state
			 * @return current state
			 */
			ButtonState state() const;
			/**
			 * Set the current button state
			 * @param s the new state
			 */
			void state(ButtonState s);
			/**
			 * Draw the button
			 * @param renderer the renderer to use for drawing
			 * @param theme the active theme to use for drawing
			 */
			void draw(ComponentRenderer * renderer, const boost::shared_ptr<Theme> & theme) const;
			/**
			 * Event notification method.
			 * Overridden from base Component. Events that buttons listen to are "enter", "leave", "button_press",
			 * "button_release"
			 * @param e the event that has occurred.
			 */
			void notify(const v3d::event::EventInfo & e);

		private:
			std::string label_;
			ButtonState state_;
			std::pair<std::string, std::string> command_; // <scope, command>
	};

}; // end namespace Luxa
