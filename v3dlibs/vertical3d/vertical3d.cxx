/**
 * Vertical|3D - 3D Content Creation Application
 * (c) 2007-2009 Josh Farr
 * Version 0.0.1
 **/

#include <cstdlib>
#include <iostream>

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
