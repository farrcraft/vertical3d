/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

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

bool ReyesPrimitive::dice(MicroPolygonGridPtr & grid)
{
	return false;
}

void ReyesPrimitive::diceable(bool status)
{
	_diceable = status;
}

