#include "FLTKKeyboard.h"

#include <iostream>

using namespace Hookah;

std::string keyEvent(unsigned char sym)
{
	std::string evnt;

	switch (sym)
	{
		case 27:
			evnt = "escape";
			break;
		case 13:
			evnt = "return";
			break;
		case 'a':
			evnt = "a";
			break;
		case 'b':
			evnt = "b";
			break;
		case 'c':
			evnt = "c";
			break;
		case 'd':
			evnt = "d";
			break;
		case 'e':
			evnt = "e";
			break;
		case 'f':
			evnt = "f";
			break;
		case 'g':
			evnt = "g";
			break;
		case 'h':
			evnt = "h";
			break;
		case 'i':
			evnt = "i";
			break;
		case 'j':
			evnt = "j";
			break;
		case 'k':
			evnt = "k";
			break;
		case 'l':
			evnt = "l";
			break;
		case 'm':
			evnt = "m";
			break;
		case 'n':
			evnt = "n";
			break;
		case 'o':
			evnt = "o";
			break;
		case 'p':
			evnt = "p";
			break;
		case 'q':
			evnt = "q";
			break;
		case 'r':
			evnt = "r";
			break;
		case 's':
			evnt = "s";
			break;
		case 't':
			evnt = "t";
			break;
		case 'u':
			evnt = "u";
			break;
		case 'v':
			evnt = "v";
			break;
		case 'w':
			evnt = "w";
			break;
		case 'x':
			evnt = "x";
			break;
		case 'y':
			evnt = "y";
			break;
		case 'z':
			evnt = "z";
			break;
		case '0':
			evnt = "0";
			break;
		case '1':
			evnt = "1";
			break;
		case '2':
			evnt = "2";
			break;
		case '3':
			evnt = "3";
			break;
		case '4':
			evnt = "4";
			break;
		case '5':
			evnt = "5";
			break;
		case '6':
			evnt = "6";
			break;
		case '7':
			evnt = "7";
			break;
		case '8':
			evnt = "8";
			break;
		case '9':
			evnt = "9";
			break;
		case '/':
			evnt = "/";
			break;
		case '.':
			evnt = ".";
			break;
		case '!':
			evnt = "!";
			break;
		case '-':
			evnt = "-";
			break;
		case ',':
			evnt = ",";
			break;
		case '+':
			evnt = "+";
			break;
		case '*':
			evnt = "*";
			break;
		case ';':
			evnt = ";";
			break;
		case '`':
			evnt = "`";
			break;
		/*
		case SDLK_CAPSLOCK:
			evnt = "capslock";
			break;
		case SDLK_TAB:
			evnt = "tab";
			break;
		case SDLK_UP:
			evnt = "arrow_up";
			break;
		case SDLK_DOWN:
			evnt = "arrow_down";
			break;
		case SDLK_LEFT:
			evnt = "arrow_left";
			break;
		case SDLK_RIGHT:
			evnt = "arrow_right";
			break;
		case SDLK_F1:
			evnt = "f1";
			break;
		case SDLK_F2:
			evnt = "f2";
			break;
		case SDLK_F3:
			evnt = "f3";
			break;
		case SDLK_F4:
			evnt = "f4";
			break;
		case SDLK_F5:
			evnt = "f5";
			break;
		case SDLK_F6:
			evnt = "f6";
			break;
		case SDLK_F7:
			evnt = "f7";
			break;
		case SDLK_F8:
			evnt = "f8";
			break;
		case SDLK_F9:
			evnt = "f9";
			break;
		case SDLK_F10:
			evnt = "f10";
			break;
		case SDLK_F11:
			evnt = "f11";
			break;
		case SDLK_F12:
			evnt = "f12";
			break;
		case SDLK_F13:
			evnt = "f13";
			break;
		case SDLK_F14:
			evnt = "f14";
			break;
		case SDLK_F15:
			evnt = "f15";
			break;
		case SDLK_LALT:
			evnt = "left_alt";
			break;
		case SDLK_RALT:
			evnt = "right_alt";
			break;
		case SDLK_RCTRL:
			evnt = "right_control";
			break;
		case SDLK_LCTRL:
			evnt = "left_control";
			break;
		case SDLK_RSHIFT:
			evnt = "right_shift";
			break;
		case SDLK_LSHIFT:
			evnt = "left_shift";
			break;
		case SDLK_INSERT:
			evnt = "insert";
			break;
		case SDLK_HOME:
			evnt = "home";
			break;
		case SDLK_DELETE:
			evnt = "delete";
			break;
		case SDLK_PAGEUP:
			evnt = "pageup";
			break;
		case SDLK_PAGEDOWN:
			evnt = "pagedown";
			break;
		*/
		case ' ':
			evnt = "space";
		default:
			break;
	}
	return evnt;
}

void FLTKKeyboard::release(unsigned char keysym)
{
	std::string evnt = keyEvent(keysym);
	state_(evnt);
	keyReleased(evnt);
}

void FLTKKeyboard::press(unsigned char keysym)
{
	std::string evnt = keyEvent(keysym);
	state_(evnt);
	keyPressed(evnt);
}

bool FLTKKeyboard::tick() {
	return true;
}