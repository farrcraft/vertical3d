/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <cassert>

#include "MicroPolygon.h"

using namespace v3D;
using namespace v3D::Moya;

MicroPolygon::MicroPolygon()
{
}

MicroPolygon::~MicroPolygon()
{
}

Vertex & MicroPolygon::operator[] (unsigned int i)
{
	assert(i < 4);
	return _points[i];
}
