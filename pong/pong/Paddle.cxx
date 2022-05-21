/**
 * Pong! - (c) Joshua Farr <j.wgasa@gmail.com>
 */

#include "Paddle.h"

Paddle::Paddle() : 
	position_(0.0f), 
	offset_(0.0f), 
	up_(false), 
	down_(false), 
	score_(0), 
	length_(50.0f), 
	size_(15.0f)
{
}

glm::vec3 Paddle::color1() const
{
	return color1_;
}

glm::vec3 Paddle::color2() const
{
	return color2_;
}


void Paddle::color(const glm::vec3 & color1, const glm::vec3 & color2)
{
    color1_ = color1;
    color2_ = color2;
}

void Paddle::move(float delta)
{
	position_ += delta;
}

void Paddle::position(const float pos)
{
	position_ = pos;
}

void Paddle::offset(const float off)
{
	offset_ = off;
}

float Paddle::position() const
{
	return position_;
}

float Paddle::offset() const
{
	return offset_;
}

void Paddle::reset()
{
/* resetting these could glitch input
	_up = false;
	_down = false;
*/
	score_ = 0;
}

bool Paddle::up()
{
	return up_;
}

bool Paddle::down()
{
	return down_;
}

int Paddle::score()
{
	return score_;
}

void Paddle::up(bool k)
{
	up_ = k;
}

void Paddle::down(bool k)
{
	down_ = k;
}

void Paddle::score(int s)
{
	score_ = s;
}

float Paddle::size()
{
	return size_;
}

float Paddle::length()
{
	return length_;
}
