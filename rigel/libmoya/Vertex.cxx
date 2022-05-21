/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#include "Vertex.h"

using namespace v3D;
using namespace v3D::Moya;

Vertex::Vertex()
{
}

Vertex::~Vertex()
{
}

Vector3 Vertex::point(void) const
{
	 return _point;
}

void Vertex::point(const Vector3 & v)
{
	 _point = v;
}

