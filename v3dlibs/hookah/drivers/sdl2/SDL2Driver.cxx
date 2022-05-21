#include "../../Hookah.h"
#include "SDL2Window.h"
#include "SDL2Keyboard.h"
#include "SDL2Mouse.h"

#ifdef _WIN32
#include <windows.h>
#include <vector>
#include <fstream>
#include <iostream>
#endif

#include <boost/shared_ptr.hpp>

boost::shared_ptr<Hookah::Window> Hookah::Create3DWindow(unsigned width, unsigned int height)
{
	boost::shared_ptr<Hookah::Window> win;
	try
	{
		std::string caption("");
		win.reset(new Hookah::SDL2Window(width, height, caption));
	}
	catch(std::exception & e)
	{
		throw e;
	}

	return win;
}

boost::shared_ptr<v3D::InputDevice> Hookah::CreateInputDevice(const std::string & device)
{
	boost::shared_ptr<v3D::InputDevice> dev;

	if (device == "keyboard")
	{
		dev.reset(new Hookah::SDL2Keyboard);
	}
	else if (device == "mouse")
	{
		dev.reset(new Hookah::SDL2Mouse);
	}

	return dev;
}

#ifdef _WIN32

/*
 * if SDL.h has been included prior to this point then SDL_main.h has been pulled 
 * in also. we're including our own entry point instead of linking in SDLmain.lib
 * so we need to work around their kludge.
 */
#undef main

extern int main(int argc, char *argv[]);

/*
 * this method's code comes from ClanLib SDK
 * Copyright (c) 1997-2005 The ClanLib Team
 * It is essentially a c++ version of the SDL ParseCommandLine() method.
 */
void CommandLineToArgv(int * argc, char **argv[])
{
	char *command_line = GetCommandLineA();
	static std::vector<char *> pos;
	bool new_arg = true;

	for (;*command_line;command_line++) 
	{
		if (*command_line == '"')
		{
			new_arg = true;
			command_line++;
			char *start_arg = command_line;
			for (;*command_line && *command_line!='"';command_line++)
			{
				if (!isspace(*command_line)) new_arg = false;
			}
			if (new_arg == false) pos.push_back(start_arg);
			if (*command_line == '"') *command_line = 0;
			else if (*command_line == 0) break;
			command_line++;
			if (*command_line == 0) break;
			new_arg = true;
		}
		
		if (new_arg && !isspace(*command_line))
		{
			new_arg = false;
			pos.push_back(command_line);
		}
		else if (!new_arg && isspace(*command_line))
		{
			new_arg = true;
			*command_line = 0;
		}
	}
	size_t num_words = pos.size();

	char **words = new char*[num_words + 1];

	size_t i;
	for (i=0; i<num_words; i++)
	{
		words[i] = pos[i];
	}
	words[i] = NULL;

	*argc = static_cast<int>(num_words);
	*argv = words;
}

INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
	char ** argv;
	int argc;
	CommandLineToArgv(&argc, &argv);

	std::ofstream out("debug.log");
	if (out)
		std::clog.rdbuf(out.rdbuf());

	return main(argc, argv);

	delete [] argv;
}

#endif // ifdef _WIN32
