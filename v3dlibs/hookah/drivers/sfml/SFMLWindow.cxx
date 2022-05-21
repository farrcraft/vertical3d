#include "SFMLWindow.h"
#include "SFMLKeyboard.h"
#include "SFMLMouse.h"

#include <SFML/Window/Event.hpp>

#include <GL/glew.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <iostream>

using namespace Hookah;


SFMLWindow::SFMLWindow(unsigned int width, unsigned int height, const std::string & cap) : 
		Window(width, height),
		bpp_(16), 
		shutdown_(false),
		dirty_(true)
{
	Window::caption(cap);

	sf::ContextSettings settings;
	// 24 bit back buffer
	settings.depthBits = 24;
	settings.stencilBits = 8;
	settings.antialiasingLevel = 4;
	// use OpenGL >= 3.2
	settings.majorVersion = 3;
	settings.minorVersion = 2;

	window_.create(sf::VideoMode(width, height), cap, sf::Style::Titlebar|sf::Style::Close, settings);

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
	window_.setVerticalSyncEnabled(true);
	window_.setActive();

	resize();
}

SFMLWindow::~SFMLWindow()
{
	shutdown();
}

void SFMLWindow::caption(const std::string & cap)
{
	Window::caption(cap);
	window_.setTitle(cap);
}

void SFMLWindow::cursor(bool state)
{
	window_.setMouseCursorVisible(state);
}

bool SFMLWindow::shutdown()
{
	if (shutdown_)
	{
		return true;
	}
	shutdown_ = true;
	return true;
}

void SFMLWindow::draw()
{
	if (shutdown_)
	{
		return;
	}
	Window::draw();
	window_.display();
	dirty_ = false;
}

void SFMLWindow::tick()
{
	if (shutdown_)
	{
		return;
	}

	// call base (will dispatch any input device events)
	Window::tick();
}

void SFMLWindow::invalidate()
{
	dirty_ = true;
}

int mapSFMLMouseButton(sf::Mouse::Button code)
{
	int button = 0;
	switch (code)
	{
		case sf::Mouse::Left: 
			button = 1;
			break;
		case sf::Mouse::Middle:
			button = 3;
			break;
		case sf::Mouse::Right:
			button = 2;
			break;
		case sf::Mouse::XButton1:
			button = 4;
			break;
		case sf::Mouse::XButton2:
			button = 5;
			break;
		default:
			button = 0;
			break;
	}
	return button;
}

std::string keyEvent(sf::Keyboard::Key sym)
{
	std::string evnt;

	switch (sym)
	{
		case sf::Keyboard::Escape:
			evnt = "escape";
			break;
		case sf::Keyboard::Return:
			evnt = "return";
			break;
		case sf::Keyboard::A:
			evnt = "a";
			break;
		case sf::Keyboard::B:
			evnt = "b";
			break;
		case sf::Keyboard::C:
			evnt = "c";
			break;
		case sf::Keyboard::D:
			evnt = "d";
			break;
		case sf::Keyboard::E:
			evnt = "e";
			break;
		case sf::Keyboard::F:
			evnt = "f";
			break;
		case sf::Keyboard::G:
			evnt = "g";
			break;
		case sf::Keyboard::H:
			evnt = "h";
			break;
		case sf::Keyboard::I:
			evnt = "i";
			break;
		case sf::Keyboard::J:
			evnt = "j";
			break;
		case sf::Keyboard::K:
			evnt = "k";
			break;
		case sf::Keyboard::L:
			evnt = "l";
			break;
		case sf::Keyboard::M:
			evnt = "m";
			break;
		case sf::Keyboard::N:
			evnt = "n";
			break;
		case sf::Keyboard::O:
			evnt = "o";
			break;
		case sf::Keyboard::P:
			evnt = "p";
			break;
		case sf::Keyboard::Q:
			evnt = "q";
			break;
		case sf::Keyboard::R:
			evnt = "r";
			break;
		case sf::Keyboard::S:
			evnt = "s";
			break;
		case sf::Keyboard::T:
			evnt = "t";
			break;
		case sf::Keyboard::U:
			evnt = "u";
			break;
		case sf::Keyboard::V:
			evnt = "v";
			break;
		case sf::Keyboard::W:
			evnt = "w";
			break;
		case sf::Keyboard::X:
			evnt = "x";
			break;
		case sf::Keyboard::Y:
			evnt = "y";
			break;
		case sf::Keyboard::Z:
			evnt = "z";
			break;
		case sf::Keyboard::Num0:
			evnt = "0";
			break;
		case sf::Keyboard::Num1:
			evnt = "1";
			break;
		case sf::Keyboard::Num2:
			evnt = "2";
			break;
		case sf::Keyboard::Num3:
			evnt = "3";
			break;
		case sf::Keyboard::Num4:
			evnt = "4";
			break;
		case sf::Keyboard::Num5:
			evnt = "5";
			break;
		case sf::Keyboard::Num6:
			evnt = "6";
			break;
		case sf::Keyboard::Num7:
			evnt = "7";
			break;
		case sf::Keyboard::Num8:
			evnt = "8";
			break;
		case sf::Keyboard::Num9:
			evnt = "9";
			break;
		case sf::Keyboard::Slash:
			evnt = "/";
			break;
		case sf::Keyboard::Period:
			evnt = ".";
			break;
		case sf::Keyboard::Dash:
			evnt = "-";
			break;
		case sf::Keyboard::Comma:
			evnt = ",";
			break;
		case sf::Keyboard::Equal:
			evnt = "=";
			break;
		case sf::Keyboard::SemiColon:
			evnt = ";";
			break;
		case sf::Keyboard::Tilde:
			evnt = "`";
			break;
		case sf::Keyboard::Tab:
			evnt = "tab";
			break;
		case sf::Keyboard::Up:
			evnt = "arrow_up";
			break;
		case sf::Keyboard::Down:
			evnt = "arrow_down";
			break;
		case sf::Keyboard::Left:
			evnt = "arrow_left";
			break;
		case sf::Keyboard::Right:
			evnt = "arrow_right";
			break;
		case sf::Keyboard::F1:
			evnt = "f1";
			break;
		case sf::Keyboard::F2:
			evnt = "f2";
			break;
		case sf::Keyboard::F3:
			evnt = "f3";
			break;
		case sf::Keyboard::F4:
			evnt = "f4";
			break;
		case sf::Keyboard::F5:
			evnt = "f5";
			break;
		case sf::Keyboard::F6:
			evnt = "f6";
			break;
		case sf::Keyboard::F7:
			evnt = "f7";
			break;
		case sf::Keyboard::F8:
			evnt = "f8";
			break;
		case sf::Keyboard::F9:
			evnt = "f9";
			break;
		case sf::Keyboard::F10:
			evnt = "f10";
			break;
		case sf::Keyboard::F11:
			evnt = "f11";
			break;
		case sf::Keyboard::F12:
			evnt = "f12";
			break;
		case sf::Keyboard::F13:
			evnt = "f13";
			break;
		case sf::Keyboard::F14:
			evnt = "f14";
			break;
		case sf::Keyboard::F15:
			evnt = "f15";
			break;
		case sf::Keyboard::LAlt:
			evnt = "left_alt";
			break;
		case sf::Keyboard::RAlt:
			evnt = "right_alt";
			break;
		case sf::Keyboard::RControl:
			evnt = "right_control";
			break;
		case sf::Keyboard::LControl:
			evnt = "left_control";
			break;
		case sf::Keyboard::RShift:
			evnt = "right_shift";
			break;
		case sf::Keyboard::LShift:
			evnt = "left_shift";
			break;
		case sf::Keyboard::Insert:
			evnt = "insert";
			break;
		case sf::Keyboard::Home:
			evnt = "home";
			break;
		case sf::Keyboard::Delete:
			evnt = "delete";
			break;
		case sf::Keyboard::PageUp:
			evnt = "pageup";
			break;
		case sf::Keyboard::PageDown:
			evnt = "pagedown";
			break;
		case sf::Keyboard::Space:
			evnt = "space";
		default:
			break;
	}
	return evnt;
}

bool SFMLWindow::nonBlockingEventLoop(DrawMode draw_mode)
{
	sf::Event event;

	int button;
	boost::shared_ptr<SFMLKeyboard> keyboard = boost::dynamic_pointer_cast<SFMLKeyboard, v3D::InputDevice>(device("keyboard"));
	boost::shared_ptr<SFMLMouse> mouse = boost::dynamic_pointer_cast<SFMLMouse, v3D::InputDevice>(device("mouse"));

	while (!shutdown_)
	{
		// handle input device events
		tick();

		// handle window events
		while (window_.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				shutdown_ = true;
			}
			else if (event.type == sf::Event::Resized)
			{
				// resize
				resize(event.size.width, event.size.height);
				invalidate();
			}
			else if (event.type == sf::Event::LostFocus)
			{
				active(false);
			}
			else if (event.type == sf::Event::GainedFocus)
			{
				active(true);
			}

			if (mouse)
			{
				if (event.type == sf::Event::MouseButtonPressed)
				{
					button = mapSFMLMouseButton(event.mouseButton.button);
					mouse->press(button);
				}
				else if (event.type == sf::Event::MouseButtonReleased)
				{
					button = mapSFMLMouseButton(event.mouseButton.button);
					mouse->release(button);
				}
				else if (event.type == sf::Event::MouseMoved)
				{
					mouse->motion(event.mouseMove.x, event.mouseMove.y);
				}
			}

			if (keyboard)
			{
				if (event.type == sf::Event::KeyPressed)
				{
					std::string evnt = keyEvent(event.key.code);
					keyboard->press(evnt);
				}
				else if (event.type == sf::Event::KeyReleased)
				{
					std::string evnt = keyEvent(event.key.code);
					keyboard->release(evnt);
				}
			}

			// catch just processed app close events
			if (shutdown_)
			{
				return true;
			}
		}

		// do any necessary window drawing
		drawFrame(draw_mode);
	}
	return true;
}

void SFMLWindow::drawFrame(DrawMode draw_mode)
{
	if (active())
	{
		static sf::Time lastTime = startTime_;
		sf::Time thisTime = clock_.getElapsedTime();

		Window::tick(thisTime.asMilliseconds() - lastTime.asMilliseconds());

		lastTime = thisTime;

		if (draw_mode == DRAW_ALWAYS || (draw_mode == DRAW_DIRTY && dirty_))
			draw();
	}
}

bool SFMLWindow::run(EventHandlingMode event_mode, DrawMode draw_mode)
{
	clock_.restart();
	startTime_ = clock_.getElapsedTime();

	if (event_mode == Window::EVENT_HANDLING_NONBLOCKING)
	{
		return nonBlockingEventLoop(draw_mode);
	}
	return true;
}
