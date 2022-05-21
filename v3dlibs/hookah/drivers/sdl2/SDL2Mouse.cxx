#include "SDL2Mouse.h"

#include <SDL.h>

using namespace Hookah;


bool SDL2Mouse::tick()
{
	SDL_Event event;
	//SDL_PumpEvents();
	while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEWHEEL) > 0)
	{
		switch (event.type)
		{
			case SDL_MOUSEMOTION:
			{
				state_(event.motion.x, event.motion.y);
				motion(event.motion.x, event.motion.y);
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			{
				unsigned int button_id = event.button.button;
				state_(button_id);
				buttonPressed(button_id);
				break;
			}
			case SDL_MOUSEBUTTONUP:
			{
				unsigned int button_id = event.button.button;
				state_(button_id);
				buttonReleased(button_id);
				break;
			}
			default:
				break;
		}
	}
	return true;
}
