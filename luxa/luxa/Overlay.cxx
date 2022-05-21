/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#include "Overlay.h"

using namespace Luxa;

Overlay::Overlay() : mode_(MODE_TRANSPARENT)
{
}

Overlay::Overlay(Mode m) : mode_(m)
{
}

Overlay::Overlay(const v3D::Color3 & c) : color_(c), mode_(MODE_COLOR)
{
}

void Overlay::color(const v3D::Color3 & c)
{
	color_  = c;
}

v3D::Color3 Overlay::color() const
{
	return color_;
}

Overlay::Mode Overlay::mode() const
{
	return mode_;
}
