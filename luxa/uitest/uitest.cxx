/**
 * libluxa uitest app
 * (c) 2007-2008 Josh Farr
 * Version 0.0.1
 * 
 * 
 * TODO:
 * 
 * ( ) text label
 * ( ) text button
 * ( ) static image icon
 * ( ) image label icon
 * ( ) image button
 * ( ) image label icon
 * ( ) icon list
 * ( ) dialog box
 * ( ) checkbox
 * ( ) radio button
 * ( ) text box
 * ( ) file chooser dialog
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
