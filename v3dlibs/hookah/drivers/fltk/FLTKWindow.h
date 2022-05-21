#ifndef INCLUDED_HOOKAH_FLTKWINDOW
#define INCLUDED_HOOKAH_FLTKWINDOW

#include "../../Window.h"

#include <fltk/GlWindow.h>

namespace Hookah
{

	/**
	 * An SDL-based Window class.
	 **/
	class FLTKWindow : public Hookah::Window, public fltk::GlWindow
	{
		public:
			FLTKWindow(unsigned int width, unsigned int height);
			~FLTKWindow();

			/**
			 * Main drawing routine called once per frame from the main game loop. This should 
			 * be overridden by the client. The parent method should not be called when overriding. 
			 */
			void draw();

			/**
			 * Set the window caption.
			 * @param cap the new window caption
			 */
			void caption(const std::string & cap);

			/**
			 * Shutdown the window. Any sdl cleanup is done here. The application will 
			 * automatically shutdown the window when it shuts down.
			 * @return true on successful shutdown, false otherwise
			 */
			bool shutdown();
			/**
			 * Primary window event loop. The application instance handles calling this method.
			 * @param event_mode whether the event loop should use polling (nonblocking) or waiting (blocking)
			 * @param draw_mode whether drawing should be done at every opportunity or only after invalidations.
			 * @return true if successful
			 */
			bool run(EventHandlingMode event_mode, DrawMode draw_mode);

			/**
			 * Event tick method overridden from base Window class.
			 */
			void tick();

			/**
			 * Mark the window contents as invalid and in need of a redraw. Overridden from base window.
			 */
			void invalidate();

			int handle(int event);

		protected:
			/**
			 * Conditionally draw a single frame. Drawing will only be done if the drawing mode conditions are 
			 * met and the window is active.
			 * @param draw_mode when to do draws.
			 */
			void drawFrame(DrawMode draw_mode);

		private:
			bool shutdown_;
			bool dirty_;
			bool active_; // window focused
	};

}; // end namespace Hookah

#endif // INCLUDED_HOOKAH_FLTKWINDOW
