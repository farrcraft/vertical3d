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
