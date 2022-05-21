#include "SDLMouse.h"

#include <SDL/SDL.h>

using namespace Hookah;


bool SDLMouse::tick()
{
	SDL_Event event;
	//SDL_PumpEvents();
	while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, 
		(SDL_EVENTMASK(SDL_MOUSEMOTION) | 
		SDL_EVENTMASK(SDL_MOUSEBUTTONDOWN) |
		SDL_EVENTMASK(SDL_MOUSEBUTTONUP))) > 0)
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
