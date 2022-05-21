/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#include "ButtonStyle.h"

using namespace Luxa;


ButtonStyle::ButtonStyle(const std::string & str, Button::ButtonState s) : Style(str, "button"), state_(s)
{
}

ButtonStyle::~ButtonStyle()
{
}

Button::ButtonState ButtonStyle::state() const
{
	return state_;
}
