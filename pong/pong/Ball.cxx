/**
 * Pong! - (c) Joshua Farr <j.wgasa@gmail.com>
 */

#include "Ball.h"

#include <cmath>


Ball::Ball() : size_(1.0f)
{
}


void Ball::direction(const glm::vec2 & dir)
{
	direction_ = dir;
}

glm::vec2 Ball::direction() const
{
	return direction_;
}

glm::vec2 Ball::position() const
{
	return position_;
}

void Ball::position(const glm::vec2 & pos)
{
	position_ = pos;
}

void Ball::move()
{
	position_ += direction_;
}

float Ball::size() const
{
	return size_;
}

void Ball::size(float s)
{
	size_ = s;
}

