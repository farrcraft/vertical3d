#include "ReyesPrimitive.h"

using namespace v3D;
using namespace v3D::Moya;

ReyesPrimitive::ReyesPrimitive()
{
}

ReyesPrimitive::~ReyesPrimitive()
{
}

bool ReyesPrimitive::diceable(void) const
{
	return _diceable;
}

AABBox ReyesPrimitive::bound(void) const
{
	return AABBox();
}

void ReyesPrimitive::split(void)
{
}

bool ReyesPrimitive::dice(boost::shared_ptr<MicroPolygonGrid> grid)
{
	return false;
}

void ReyesPrimitive::diceable(bool status)
{
	_diceable = status;
}

