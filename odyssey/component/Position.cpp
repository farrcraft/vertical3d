/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Position.h"

using namespace odyssey::component;

Position::Position(const int x, const int y) noexcept
	: position_(x, y) {
}

Position::Position(Position&& p) noexcept {
	this->position_ = p.position_;
}

Position& Position::operator=(Position&& p) noexcept
{
	if (this != &p) {
		this->position_ = p.position_;
	}

	return *this;
}
