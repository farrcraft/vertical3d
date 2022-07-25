/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "Vertex.h"

using namespace v3d::brep;

Vertex::Vertex()
{
}

Vertex::Vertex(const glm::vec3& p) : point_(p)
{
}

Vertex::~Vertex()
{
}

bool Vertex::operator == (const Vertex & v)
{
	return (point_ == v.point_);
}

bool Vertex::operator == (const glm::vec3 & v)
{
	return (point_ == v);
}

unsigned int Vertex::edge(void) const
{
	return edge_;
}

void Vertex::edge(unsigned int e)
{
	edge_ = e;
}

glm::vec3 Vertex::point(void) const
{
	return point_;
}

void Vertex::point(const glm::vec3& p)
{
	point_ = p;
}
