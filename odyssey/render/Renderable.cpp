/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Renderable.h"

using namespace odyssey::render;

/**
 **/
Renderable::Renderable(boost::shared_ptr<Engine> renderer) :
	renderer_(renderer) {
}
