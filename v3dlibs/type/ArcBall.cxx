/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "ArcBall.h"
#include <cmath>

using namespace v3d::type;

ArcBall::ArcBall()
{
}

ArcBall::~ArcBall()
{
}

Vector3 ArcBall::map(const Vector2 & new_point)
{
	Vector3 mapped_point;
	Vector2 point;
	point = new_point;

	// scale point to range [-1, 1]
	point[0] = (point[0] * width_) - 1.0f;
	point[1] = 1.0f - (point[1] * height_);

	float length;
	// squared length
	length = (point[0] * point[0]) + (point[1] * point[1]);

	// point mapped outside sphere (length > radius^2)
	if (length > 1.0f)
	{
		float norm;
		norm = 1.0f / sqrtf(length);

		mapped_point[0] = point[0] * norm;
		mapped_point[1] = point[1] * norm;
		mapped_point[2] = 0.0f;
	}
	else // point mapped inside sphere
	{
		mapped_point[0] = point[0];
		mapped_point[1] = point[1];
		mapped_point[2] = sqrtf(1.0f - length);
	}
	return mapped_point;
}

void ArcBall::click(const Vector2 & point)
{
	start_ = map(point);
}

Quaternion ArcBall::drag(const Vector2 & point)
{
	end_ = map(point);

	Quaternion rot;
	Vector3 perp;

	perp = start_.cross(end_);

	if (perp.length() > EPSILON)
	{
		rot[0] = perp[0];
		rot[1] = perp[1];
		rot[2] = perp[2];
		rot[3] = start_.dot(end_);
	}
	else
	{
		rot[0] = rot[1] = rot[2] = rot[3] = 0.0f;
	}
	start_ = end_;
	return rot;
}

void ArcBall::bounds(float width, float height)
{
	if (width < 1.0f)
		width = 1.0f;
	if (height < 1.0f)
		height = 1.0f;
	width_ = 1.0f / ((width - 1.0f) * 0.5f);
	height_ = 1.0f / ((height - 1.0f) * 0.5f);
}
