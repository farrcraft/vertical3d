/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#include "Face.h"

using namespace v3D;

Face::Face() : _selected(false)
{
}

Face::Face(const Vector3 & normal, unsigned int edgeID) : _normal(normal), _edgeID(edgeID), _selected(false)
{
}

Face::~Face()
{
}

bool Face::selected(void) const
{
	return _selected;
}

void Face::selected(bool sel)
{
	_selected = sel;
}

Vector3 Face::normal(void) const
{
	return _normal;
}

void Face::normal(const Vector3 & n)
{
	_normal = n;
}

unsigned int Face::edge(void) const
{
	return _edgeID;
}

void Face::edge(unsigned int e)
{
	_edgeID = e;
}
