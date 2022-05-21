#include "FLTKWindow.h"
#include "FLTKKeyboard.h"
#include "FLTKMouse.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <iostream>

#include <fltk/visual.h>
#include <fltk/events.h>
#include <fltk/run.h>

using namespace Hookah;

FLTKWindow::FLTKWindow(unsigned int width, unsigned int height) : 
		Hookah::Window(width, height),
		shutdown_(false),
		dirty_(true), 
		fltk::GlWindow(width, height)
{
	mode(fltk::DEPTH_BUFFER | fltk::RGB_COLOR | fltk::DOUBLE_BUFFER | fltk::MULTISAMPLE);
	resizable(this);
	show();
	valid(0);
	make_current();
	Window::resize();
}

FLTKWindow::~FLTKWindow()
{
	shutdown();
}

void FLTKWindow::caption(const std::string & cap)
{
	Window::caption(cap);
	label(caption_.c_str());
}

bool FLTKWindow::shutdown()
{
	if (shutdown_)
	{
		return true;
	}
	shutdown_ = true;
	return true;
}

void FLTKWindow::draw()
{
	if (shutdown_)
	{
		return;
	}
	Window::draw();
	swap_buffers();
	dirty_ = false;
}

void FLTKWindow::tick()
{
	if (shutdown_)
	{
		return;
	}

	// call base (will dispatch any input device events)
	Window::tick();
}

int FLTKWindow::handle(int event) 
{
	make_current();
	int ex = fltk::event_x();
	int ey = fltk::event_y();
	int button;
	boost::shared_ptr<FLTKKeyboard> keyboard = boost::dynamic_pointer_cast<FLTKKeyboard, v3D::InputDevice>(device("keyboard"));
	boost::shared_ptr<FLTKMouse> mouse = boost::dynamic_pointer_cast<FLTKMouse, v3D::InputDevice>(device("mouse"));

	switch (event) 
	{
		case fltk::PUSH:
			if (keyboard)
				focus(this);
			button = fltk::event_button() - 1;
			if (button < 0) 
				button = 0;
			if (button > 2) 
				button = 2;
			if (mouse)
			{
				mouse->press(button);
				return 1;
			}
			break;

		case fltk::MOUSEWHEEL:
			button = fltk::event_dy();
			while (button < 0) 
			{
				if (mouse)
					mouse->press(3);
				++button;
			}
			while (button > 0) 
			{
				if (mouse)
					mouse->press(4);
				--button;
			}
			return 1;
			break;

		case fltk::RELEASE:
			button = fltk::event_button();
			if (mouse)
				mouse->release(button);
			/*
			for (button = 0; button < 3; button++)
			{
				if (mouse_down & (1<<button)) 
				{
				  if (mouse) 
					  mouse->release(button);
					  //mouse(button,GLUT_UP,ex,ey);
				}
			}
			mouse_down = 0;
			*/
			return 1;
/*
		case ENTER:
		    if (entry) 
			{
				entry(GLUT_ENTERED); 
				return 1;
			}
			if (passivemotion) 
				return 1;
			break;

		case LEAVE:
			if (entry) 
			{
				entry(GLUT_LEFT); 
				return 1;
			}
			if (passivemotion) 
				return 1;
			break;
*/
		case fltk::DRAG:
			if (mouse) 
			{
				mouse->motion(ex, ey);
				return 1;
			}
			break;
		case fltk::MOVE:
			if (mouse) 
			{
				mouse->motion(ex, ey);
				return 1;
			}
			break;

		case fltk::FOCUS:
			if (keyboard) 
				return 1;
			break;

		case fltk::KEY:
		case fltk::SHORTCUT:
			if (fltk::event_text()[0]) 
			{
				if (keyboard) 
				{
					keyboard->press(fltk::event_text()[0]);
					//keyboard(event_text()[0],ex,ey); 
					return 1;
				}
				  break;
			} 
			else 
			{
				if (keyboard) 
				{
					int k = fltk::event_key();
					if (k > fltk::F0Key && k <= fltk::LastFunctionKey) 
						k = k - fltk::F0Key;
					//keyboard->press(k);
					//special(k,ex,ey);
					return 1;
				}
				break;
			}

		case fltk::HIDE:
			active_ = false;
			break;

		case fltk::SHOW:
			active_ = true;
			break;
	}

  return GlWindow::handle(event);
}

void FLTKWindow::invalidate()
{
	dirty_ = true;
}

unsigned int startTime = 0;

void FLTKWindow::drawFrame(DrawMode draw_mode)
{
	if (active_)
	{
		unsigned int thisTime = 0;
		static unsigned int lastTime = startTime;
		unsigned int totalFrames = 0;
		unsigned int fps = 0;

//		thisTime = SDL_GetTicks();
		totalFrames++;
		fps = (thisTime - startTime) / totalFrames * 1000;
//			std::cout << fps << "fps.d(" << delta << ")-frames(" << totalFrames << ")" << std::endl;

		Window::tick(thisTime - lastTime);

		lastTime = thisTime;

		if (draw_mode == DRAW_ALWAYS || (draw_mode == DRAW_DIRTY && dirty_))
			draw();
	}
}

bool FLTKWindow::run(EventHandlingMode event_mode, DrawMode draw_mode)
{
	fltk::run();

	return true;
}
