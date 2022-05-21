#include "SDLWindow.h"
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <iostream>

using namespace Hookah;


SDLWindow::SDLWindow(unsigned int width, unsigned int height) : 
		Window(width, height),
		surface_(0), 
		bpp_(16), 
		shutdown_(false),
		dirty_(true)
{
	// SDL initialization
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::string err = std::string("video initialization failed: ") + SDL_GetError();
		throw std::runtime_error(err);
	}
	const SDL_VideoInfo * video_info;
	video_info = SDL_GetVideoInfo();

	if (!video_info)
	{
		std::string err = std::string("video query failed: ") + SDL_GetError();
		throw std::runtime_error(err);
	}

	videoFlags_ = SDL_OPENGL|SDL_GL_DOUBLEBUFFER|SDL_HWPALETTE|SDL_RESIZABLE;

	if (video_info->hw_available)
		videoFlags_ |= SDL_HWSURFACE;
	else
		videoFlags_ |= SDL_SWSURFACE;

	if (video_info->blit_hw)
		videoFlags_ |= SDL_HWACCEL;

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	surface_ = SDL_SetVideoMode(Window::width(), Window::height(), bpp_, videoFlags_);

	if (!surface_)
	{
		std::string err = std::string("video mode set failed: ") + SDL_GetError();
		throw std::runtime_error(err);
	}

	// set window caption
	SDL_WM_SetCaption(caption_.c_str(), NULL);

	resize();
}

SDLWindow::~SDLWindow()
{
	shutdown();
}

void SDLWindow::caption(const std::string & cap)
{
	Window::caption(cap);
	// only set caption if window has already been initialized
	if (surface_)
		SDL_WM_SetCaption(caption_.c_str(), NULL);
}

bool SDLWindow::shutdown()
{
	if (shutdown_)
		return true;
	shutdown_ = true;
	SDL_Quit();
	return true;
}

void SDLWindow::draw()
{
	if (shutdown_)
		return;
	Window::draw();
	SDL_GL_SwapBuffers();
	dirty_ = false;
}

void SDLWindow::tick()
{
	if (shutdown_)
		return;

	// call base (will dispatch any input device events)
	Window::tick();
}

void SDLWindow::invalidate()
{
	dirty_ = true;
}

bool SDLWindow::blockingEventLoop(DrawMode draw_mode)
{
	// this loop is essentially our own implementation of SDL_WaitEvent()
	int status;
	SDL_Event event;
	while (!shutdown_)
	{
		// pump the message loop
		SDL_PumpEvents();
		// look for window events
		status = SDL_PeepEvents(&event, 1, SDL_GETEVENT, 
			SDL_EVENTMASK(SDL_ACTIVEEVENT) | 
			SDL_EVENTMASK(SDL_VIDEORESIZE) | 
			SDL_EVENTMASK(SDL_QUIT) | 
			SDL_EVENTMASK(SDL_VIDEOEXPOSE));
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

bool SDLWindow::nonBlockingEventLoop(DrawMode draw_mode)
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

bool SDLWindow::dispatchSDLEvent(const SDL_Event & event)
{
	switch (event.type)
	{
		case SDL_ACTIVEEVENT:
			if (!(event.active.state & SDL_APPMOUSEFOCUS))
			{
				if (event.active.gain == 0)
					active_ = false;
				else
					active_ = true;
			}
			break;
		case SDL_VIDEOEXPOSE:
			break;
		case SDL_VIDEORESIZE:
			surface_ = SDL_SetVideoMode(event.resize.w, event.resize.h, 16, videoFlags_);
			if (!surface_)
			{
				std::cerr << "couldn't get surface after resize: " << SDL_GetError() << std::endl;
				return false;
			}
			resize(event.resize.w, event.resize.h);
			invalidate();
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

void SDLWindow::drawFrame(DrawMode draw_mode)
{
	if (active_)
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

bool SDLWindow::run(EventHandlingMode event_mode, DrawMode draw_mode)
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
