/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "Face.h"

using namespace v3d::brep;

Face::Face()
{
}

Face::Face(const v3d::type::Vector3 & normal, unsigned int edge) : normal_(normal), edge_(edge)
{
}

Face::~Face()
{
}

v3d::type::Vector3 Face::normal(void) const
{
	return normal_;
}

void Face::normal(const v3d::type::Vector3 & n)
{
	normal_ = n;
}

unsigned int Face::edge(void) const
{
	return edge_;
}

void Face::edge(unsigned int e)
{
	edge_ = e;
}
