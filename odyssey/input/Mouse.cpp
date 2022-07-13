/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Mouse.h"

using namespace odyssey::input;

/**
 **/
bool Mouse::handleEvent(const SDL_Event& event)
{
	switch (event.type) {
	case SDL_MOUSEBUTTONUP:
		break;
	case SDL_MOUSEBUTTONDOWN:
		break;
	case SDL_MOUSEMOTION:
		break;
	default:
		return false;
	}
	return true;
}