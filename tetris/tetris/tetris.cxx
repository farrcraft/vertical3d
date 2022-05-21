/**
 * Tetris - (c) 2006-2007 Josh Farr
 *
 * http://en.wikipedia.org/wiki/Tetris
 *
 * TODO:
 * (*) draw tetrads
 * (*) draw gameboard
 * (*) get random tetrad
 * (*) current tetrad gravity
 * (*) move tetrads
 * (*) tetrad / board collision detection
 * (*) break apart tetrads
 * (*) rotate tetrads
 * (*) clear full rows
 * (*) random tetrad start orientation
 * (*) preview next tetrad
 * (*) drop partial rows after clearing full rows
 * (*) fix disappearing tetrad when moving left
 * (*) fast drop tetrads
 * (*) flip screen coordinates vertically
 * ( ) scoring
 * ( ) check collisions when moving tetrads left/right (in scene::exec method)
 **/

#include <cstdlib>

#include "Controller.h"

int main (int argc, char *argv[])
{
	Controller controller;

	if (!controller.run())
	{
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
