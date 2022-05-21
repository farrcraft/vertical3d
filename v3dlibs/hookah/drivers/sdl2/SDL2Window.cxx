#include "SDL2Window.h"

#include <sstream>
#include <iostream>

#include <GL/glew.h>

#include <boost/function.hpp>

using namespace Hookah;


SDL2Window::SDL2Window(unsigned int width, unsigned int height, const std::string & cap) : 
		Window(width, height),
		window_(0), 
		bpp_(16), 
		shutdown_(false),
		dirty_(true)
{
	Window::caption(cap);

	// SDL initialization
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::string err = std::string("video initialization failed: ") + SDL_GetError();
		throw std::runtime_error(err);
	}

	// use OpenGL >= 3.2
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	// 24 bit back buffer
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	window_ = SDL_CreateWindow(cap.c_str(), 
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
		width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
	);
	if (!window_)
	{
		SDL_Quit();
		std::string err = std::string("window creation failed: ") + SDL_GetError();
		throw std::runtime_error(err);
	}
	
	context_ = SDL_GL_CreateContext(window_);

	// need the experimental flag to get support for glGenVertexArrays
	glewExperimental = GL_TRUE; 
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		std::stringstream msg;
		msg << glewGetErrorString(err);
		throw std::runtime_error(msg.str());
	}

	// enable vsync
	SDL_GL_SetSwapInterval(1);	

	resize();
}

SDL2Window::~SDL2Window()
{
	shutdown();
}

void SDL2Window::caption(const std::string & cap)
{
	Window::caption(cap);
	SDL_SetWindowTitle(window_, cap.c_str());
}

void SDL2Window::cursor(bool state)
{
	int toggle = state ? 1 : 0;
	SDL_ShowCursor(toggle);
}

void SDL2Window::warpCursor(int x, int y)
{
	SDL_WarpMouseInWindow(window_, x, y);
}

bool SDL2Window::shutdown()
{
	if (shutdown_)
	{
		return true;
	}
	shutdown_ = true;
	SDL_GL_DeleteContext(context_);
	SDL_DestroyWindow(window_);
	SDL_Quit();
	return true;
}

void SDL2Window::draw()
{
	if (shutdown_)
	{
		return;
	}
	Window::draw();
	SDL_GL_SwapWindow(window_);
	dirty_ = false;
}

void SDL2Window::tick()
{
	if (shutdown_)
	{
		return;
	}

	// call base (will dispatch any input device events)
	Window::tick();
}

void SDL2Window::invalidate()
{
	dirty_ = true;
}

bool SDL2Window::blockingEventLoop(DrawMode draw_mode)
{
	// this loop is essentially our own implementation of SDL_WaitEvent()
	int status;
	SDL_Event event;
	while (!shutdown_)
	{
		// pump the message loop
		SDL_PumpEvents();
		// look for window events
		status = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_QUIT, SDL_SYSWMEVENT);
		// handle a queued window event if we found one
		if (status == 1)
		{
			// process the event
			if (!dispatchSDLEvent(event))
				return false;
		} 
		// handle input device events
		tick();

		// catch any just processed app close events
		if (shutdown_)
			return true;

		// do any necessary window drawing
		drawFrame(draw_mode);

		// there were no events to process so go to sleep for a while
		if (status == 0)
		{
			SDL_Delay(10);
		}
		else if (status == -1) // epic fail
		{
			return false;
		}
	}
	return true;
}

bool SDL2Window::nonBlockingEventLoop(DrawMode draw_mode)
{
	SDL_Event event;
	while (!shutdown_)
	{
		// handle input device events
		tick();

		// handle window events
		while(SDL_PollEvent(&event))
		{
			// process the event
			if (!dispatchSDLEvent(event))
				return false;
			// catch just processed app close events
			if (shutdown_)
				return true;
		}

		// do any necessary window drawing
		drawFrame(draw_mode);
	}
	return true;
}

bool SDL2Window::dispatchSDLEvent(const SDL_Event & event)
{
	switch (event.type)
	{
		case SDL_WINDOWEVENT:
			// See: http://wiki.libsdl.org/moin.cgi/SDL_WindowEvent?highlight=%28%5CbCategoryStruct%5Cb%29%7C%28CategoryVideo%29%7C%28SGStructures%29
			switch (event.window.event)
			{
				case SDL_WINDOWEVENT_RESIZED:
					SDL_SetWindowSize(window_, event.window.data1, event.window.data2);
					resize(event.window.data1, event.window.data2);
					invalidate();
					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
					active(false);
					break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
					active(true);
					break;
			}
			break;
		case SDL_SYSWMEVENT:
			break;
		case SDL_QUIT:
			shutdown_ = true;
			break;
		default:
			break;
	}
	return true;
}

unsigned int startTime = 0;

void SDL2Window::drawFrame(DrawMode draw_mode)
{
	if (active())
	{
		unsigned int thisTime = 0;
		static unsigned int lastTime = startTime;
		unsigned int totalFrames = 0;
		unsigned int fps = 0;

		thisTime = SDL_GetTicks();
		totalFrames++;
		fps = (thisTime - startTime) / totalFrames * 1000;
//			std::cout << fps << "fps.d(" << delta << ")-frames(" << totalFrames << ")" << std::endl;

		Window::tick(thisTime - lastTime);

		lastTime = thisTime;

		if (draw_mode == DRAW_ALWAYS || (draw_mode == DRAW_DIRTY && dirty_))
			draw();
	}
}

bool SDL2Window::run(EventHandlingMode event_mode, DrawMode draw_mode)
{
	startTime = SDL_GetTicks();

	if (event_mode == Window::EVENT_HANDLING_NONBLOCKING)
	{
		return nonBlockingEventLoop(draw_mode);
	}
	else // blocking
	{
		return blockingEventLoop(draw_mode);
	}
	return true;
}
