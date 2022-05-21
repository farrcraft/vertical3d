#include "Window.h"

#include <iostream>

using namespace Hookah;

Window::Window(unsigned int width, unsigned int height) : 
	width_(width), 
	height_(height),
	active_(true)
{
}

Window::~Window()
{
}

void Window::caption(const std::string & cap)
{
	caption_ = cap;
}

void Window::cursor(bool state)
{
}

void Window::warpCursor(int x, int y)
{
}


int Window::width() const
{
	return width_;
}

int Window::height() const
{
	return height_;
}

void Window::resize(int w, int h)
{
	width_ = w;
	height_ = h;
	resize();
}

void Window::resize()
{
	resizeEventListeners_(width_, height_);
}

bool Window::active() const
{
	return active_;
}

void Window::active(bool state)
{
	active_ = state;
}

void Window::draw()
{
	drawEventListeners_(this);
	// here is where we render overlays (if the luxa listeners have been bound)
	postDrawEventListeners_(this);
}

void Window::addDrawListener(const DrawEventType::slot_function_type & slot)
{
	drawEventListeners_.connect(slot);
}

void Window::addResizeListener(const ResizeEventType::slot_function_type & slot)
{
	resizeEventListeners_.connect(slot);
}

void Window::addPostDrawListener(const DrawEventType::slot_function_type & slot)
{
	postDrawEventListeners_.connect(slot);
}

void Window::addTickListener(const TickEventType::slot_function_type & slot)
{
	tickEventListeners_.connect(slot);
}

void Window::tick(unsigned int delta)
{
	tickEventListeners_(delta);
}

boost::shared_ptr<v3D::InputDevice> Window::device(const std::string & name)
{
	boost::shared_ptr<v3D::InputDevice> empty_ptr;
	std::map< std::string, boost::shared_ptr<v3D::InputDevice> >::iterator iter = inputDevices_.find(name);
	if (iter != inputDevices_.end())
		return iter->second;
	return empty_ptr;
}

void Window::tick()
{
	std::map< std::string, boost::shared_ptr<v3D::InputDevice> >::iterator iter = inputDevices_.begin();
	for (; iter != inputDevices_.end(); ++iter)
	{
		iter->second->tick();
	}
}

void Window::addInputDevice(const std::string & name, boost::shared_ptr<v3D::InputDevice> device)
{
	inputDevices_[name] = device;
}

/*
bool Window::exec(const std::string & scope, const std::string & command)
{
	else if (command == "toggleFS")
	{
		SDL_WM_ToggleFullScreen(_surface);
		return false;
	}
	return true;
}

*/