/*
	Copyright (C) 2001-2004 by Josh Farr
	merkaba_at_quantumfish_dot_com

	This file is part of Vertical|3D.

	Vertical|3D is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Vertical|3D is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Vertical|3D; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "Transform.h"

using namespace v3D;
using namespace v3D::DAG;

Transform::Transform() : _translation(0.0, 0.0, 0.0), _scale(1.0, 1.0, 1.0)
{
}

Transform::~Transform()
{
}

void Transform::scale(const Vector3 & s)
{
	_scale = s;
}

void Transform::rotation(const Quaternion & r)
{
	_rotation = r;
}

void Transform::translation(const Vector3 & t)
{
	_translation += t;
}

Vector3 Transform::scale(void) const
{
	return _scale;
}

Quaternion Transform::rotation(void) const
{
	return _rotation;
}

Vector3 Transform::translation(void) const
{
	return _translation;
}

Matrix4 Transform::matrix(void) const
{
	Matrix4 trans;
	trans.identity();
	trans.translate(_translation[0], _translation[1], _translation[2]);
	trans.scale(_scale[0], _scale[1], _scale[2]);
	trans *= _rotation.matrix();
	return trans;
}
