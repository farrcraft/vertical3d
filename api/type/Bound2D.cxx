/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "Bound2D.h"

using namespace v3d::type;

Bound2D::Bound2D(float x, float y, float width, float height) : size_(width, height), position_(x, y)
{
}

Bound2D::Bound2D(const glm::vec2 & position, const glm::vec2 & size) : size_(size), position_(position)
{
}

void Bound2D::shrink(float size)
{
	// shrink the bounds 
	size_[0] -= size;
	size_[1] -= size;
	// adjust the origin so the midpoint doesn't move
	position_[0] += size;
	position_[1] += size;
}

void Bound2D::expand(float size)
{
	// expand the bounds 
	size_[0] += size;
	size_[1] += size;
	// adjust the origin so the midpoint doesn't move
	position_[0] += size;
	position_[1] += size;
}

bool Bound2D::intersect(const glm::vec2 & point)
{
	if ((point[0] >= position_[0]) && 
		(point[1] >= position_[1]) && 
		(point[0] <= (position_[0] + size_[0])) && 
		(point[1] <= (position_[1] + size_[1])))
		return true;
	return false;
}


Bound2D & Bound2D::operator += (const Bound2D & bound)
{
	size_ += bound.size();
	return *this;
}


glm::vec2 Bound2D::size() const
{
	return size_;
}

glm::vec2 Bound2D::position() const
{
	return position_;
}

