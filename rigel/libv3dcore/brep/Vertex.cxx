/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#include "Vertex.h"

using namespace v3D;

Vertex::Vertex()
{
}

Vertex::Vertex(const Vector3 & p) : _point(p), _selected(false)
{
}

Vertex::~Vertex()
{
}

bool Vertex::operator == (const Vertex & v)
{
	return (_point == v._point);
}

bool Vertex::operator == (const Vector3 & v)
{
	return (_point == v);
}

bool Vertex::selected(void) const
{
	return _selected;
}

void Vertex::selected(bool sel)
{
	_selected = sel;
}

unsigned int Vertex::edge(void) const
{
	return _edgeID;
}

void Vertex::edge(unsigned int e)
{
	_edgeID = e;
}

Vector3 Vertex::point(void) const
{
	return _point;
}

void Vertex::point(const Vector3 & p)
{
	_point = p;
}
