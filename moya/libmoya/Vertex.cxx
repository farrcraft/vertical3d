/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Vertex.h"

using namespace v3d::moya;

Vertex::Vertex()
{
}

Vertex::~Vertex()
{
}

glm::vec3 Vertex::point(void) const
{
     return point_;
}

void Vertex::point(const glm::vec3 & v)
{
     point_ = v;
}

