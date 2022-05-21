#ifndef INCLUDED_HOOKAH_SFMLWINDOW
#define INCLUDED_HOOKAH_SFMLWINDOW

#include "../../Window.h"

#include <SFML/Window/Window.hpp>

namespace Hookah
{

	/**
	 * An SFML-based Window class.
	 **/
	class SFMLWindow : public Window
	{
		public:
			SFMLWindow(unsigned int width, unsigned int height, const std::string & cap);
			~SFMLWindow();

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
			 * Toggle mouse cursor visibility
			 * @param state whether to enable or disable
			 */
			void cursor(bool state);

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

		protected:
			/**
			 * Internal method to run the core non-blocking event loop.
			 * @param draw_mode whether drawing should be done at every opportunity or only after invalidations.
			 * @return true if successful
			 */
			bool nonBlockingEventLoop(DrawMode draw_mode);

			/**
			 * Conditionally draw a single frame. Drawing will only be done if the drawing mode conditions are 
			 * met and the window is active.
			 * @param draw_mode when to do draws.
			 */
			void drawFrame(DrawMode draw_mode);

		private:
			sf::Window window_;
			sf::Time startTime_;
			sf::Clock clock_;
			int bpp_;
			int videoFlags_;
			bool shutdown_;
			bool dirty_;
	};

}; // end namespace Hookah

#endif // INCLUDED_HOOKAH_SFMLWINDOW
