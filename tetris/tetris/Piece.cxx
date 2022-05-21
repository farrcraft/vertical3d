#include "Piece.h"

#include <iostream>

Piece::Piece(ColorType type) : color_(type)
{
}

Piece::Piece(std::string color_name)
{
	if (color_name == "red")
		color_ = COLOR_RED;
	else if (color_name == "cyan")
		color_ = COLOR_CYAN;
	else if (color_name == "blue")
		color_ = COLOR_BLUE;
	else if (color_name == "green")
		color_ = COLOR_GREEN;
	else if (color_name == "orange")
		color_ = COLOR_ORANGE;
	else if (color_name == "purple")
		color_ = COLOR_PURPLE;
	else if (color_name == "yellow")
		color_ = COLOR_YELLOW;
	else
		color_ = COLOR_EMPTY;
}

Piece::ColorType Piece::color() const
{
	return color_;
}

std::string Piece::str() const
{
	switch (color_)
	{
		case COLOR_EMPTY: 
			return "empty";
			break;
		case COLOR_RED:
			return "red";
			break;
		case COLOR_CYAN:
			return "cyan";
			break;
		case COLOR_BLUE:
			return "blue";
			break;
		case COLOR_GREEN:
			return "green";
			break;
		case COLOR_ORANGE:
			return "orange";
			break;
		case COLOR_PURPLE:
			return "purple";
			break;
		case COLOR_YELLOW:
			return "yellow";
			break;
		default:
			return "";
			break;
	}
	return "";
}
