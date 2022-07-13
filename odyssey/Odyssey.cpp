/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include <SDL.h>
#include <SDL_main.h>

#include "Odyssey.h"
#include "engine/Engine.h"

/**
 **/
int main(int argc, char* argv[]) {
	odyssey::engine::Engine engine;
	if (!engine.initialize()) {
		return -1;
	}

	const bool ok = engine.run();

	if (!engine.shutdown()) {
		return -1;
	}

	if (!ok) {
		return -1;
	}

	return 0;
}
