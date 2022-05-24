/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../input/InputDevice.h"

#include <boost/shared_ptr.hpp>
#include <boost/signals2/signal.hpp>

#include <vector>

namespace Hookah
{

	/**
	 * An abstract base Application Window class.
	 * 
	 */
	class Window
	{
		public:
			Window(unsigned int width, unsigned int height);
			virtual ~Window();

			typedef enum 
			{
				EVENT_HANDLING_BLOCKING,
				EVENT_HANDLING_NONBLOCKING
			} EventHandlingMode;

			typedef enum
			{
				DRAW_NONE,
				DRAW_DIRTY,
				DRAW_ALWAYS
			} DrawMode;

			/**
			 * Main drawing routine called once per frame from the main game loop. This should 
			 * be overridden by the client. The parent method should not be called when overriding. 
			 */
			virtual void draw();
			/**
			 * Window update method. This method is called once per frame to handle input processing
			 * and event dispatching.
			 */
			virtual void tick();
			/**
			 * Set the window caption.
			 * @param cap the new window caption
			 */
			virtual void caption(const std::string & cap);
			/**
			 * Toggle mouse cursor visibility
			 * @param state whether to enable or disable
			 */
			virtual void cursor(bool state);
			/**
			 * Move the mouse cursor to a new position in the window
			 */
			virtual void warpCursor(int x, int y);

			/**
			 * Get the width of the window.
			 * @return the width of the window
			 */
			int width() const;
			/**
			 * Get the height of the window.
			 * @return the height of the window
			 */
			int height() const;

			bool active() const;

			/**
			 * Shutdown the window. Any SDL, audio cleanup is done here. The application will 
			 * automatically shutdown the window when it shuts down.
			 * @return true on successful shutdown, false otherwise
			 */
			virtual bool shutdown() = 0;
			/**
			 * Primary window event loop. The application instance handles calling this method.
			 * @param event_mode whether the event loop should be blocking or not
			 * @param draw_mode how often to do draws
			 * @return true if successful
			 */
			virtual bool run(EventHandlingMode event_mode, DrawMode draw_mode = DRAW_ALWAYS) = 0;

			/**
			 * Add an input device to the window
			 * @param name the name of the input device - should match the name used to create the device
			 * @param device an input device object
			 */
			void addInputDevice(const std::string & name, boost::shared_ptr<v3d::input::InputDevice> device);

			typedef boost::signals2::signal<void (unsigned int) > TickEventType;
			typedef boost::signals2::signal< void (Window *) > DrawEventType;
			typedef boost::signals2::signal< void (int, int) > ResizeEventType;

			void addDrawListener(const DrawEventType::slot_function_type & slot);
			void addResizeListener(const ResizeEventType::slot_function_type & slot);
			void addPostDrawListener(const DrawEventType::slot_function_type & slot);
			void addTickListener(const TickEventType::slot_function_type & slot);

			/**
			 * Heartbeat method called once per frame.
			 * @param delta the amount of time in milliseconds that has passed since the last tick.
			 */
			void tick(unsigned int delta);

			/**
			 * Mark the window contents as invalid and in need of a redraw
			 */
			virtual void invalidate() = 0;

		protected:
			/**
			 * Resize the window to the current width and height settings.
			 **/
			void resize();
			void resize(int w, int h);
			std::string caption_;
			void active(bool state);

			boost::shared_ptr<v3d::input::InputDevice> device(const std::string & name);

		private:
			int width_;
			int height_;
			std::map< std::string, boost::shared_ptr<v3d::input::InputDevice> > inputDevices_;
			bool active_; // window focused

			TickEventType tickEventListeners_;
			DrawEventType drawEventListeners_;
			ResizeEventType resizeEventListeners_;
			DrawEventType postDrawEventListeners_;
	};

}; // end namespace Hookah
