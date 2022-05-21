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
#include <cassert>
#include <cmath>
#include <sstream>

#include "Quaternion.h"

using namespace v3D;

Quaternion::Quaternion()
{
	_quat[0] = 0.;
	_quat[1] = 0.;
	_quat[2] = 0.;
	_quat[3] = 1.;
}

Quaternion::Quaternion(const Quaternion & q)
{
	_quat[0] = q._quat[0];
	_quat[1] = q._quat[1];
	_quat[2] = q._quat[2];
	_quat[3] = q._quat[3];
}

Quaternion::Quaternion(real_t q[4])
{
	_quat[0] = q[0];
	_quat[1] = q[1];
	_quat[2] = q[2];
	_quat[3] = q[3];
}

Quaternion::Quaternion(real_t x, real_t y, real_t z, real_t w)
{
	_quat[0] = x;
	_quat[1] = y;
	_quat[2] = z;
	_quat[3] = w;
}

Quaternion::Quaternion(const Matrix4 & mat)
{
	real_t angle_x, angle_y, angle_z, tr_x, tr_y, C, D;

	angle_y = D = asin(mat[2]);
	C 			= cos(angle_y);
	angle_y = rad2deg(angle_y);
	if (fabs(C) > 0.005)
	{
		tr_x = mat[10] / C;
		tr_y = -mat[6] / C;
		angle_x = rad2deg(atan2(tr_y, tr_x));
		tr_x = mat[0] / C;
		tr_y = -mat[1] / C;
		angle_z = rad2deg(atan2(tr_y, tr_x));
	}
	else
	{
		angle_x = 0.0;
		tr_x = mat[5];
		tr_y = mat[4];
		angle_z = rad2deg(atan2(tr_y, tr_x));
	}

	if (angle_x < 0.0)
		angle_x += 360.0;
	if (angle_y < 0.0)
		angle_y += 360.0;
	if (angle_z < 0.0)
		angle_z += 360.0;

	euler(angle_x, angle_y, angle_z);
}

Quaternion::~Quaternion()
{
}

Quaternion Quaternion::conjugate(void) const
{
	Quaternion q(-_quat[0], -_quat[1], -_quat[2], _quat[3]);
	return q;
}

real_t Quaternion::magnitude(void) const
{
	return sqrt(_quat[3] * _quat[3] + _quat[0] * _quat[0] + _quat[1] * _quat[1] + _quat[2] * _quat[2]);
}

void Quaternion::normalize(void)
{
	real_t m = magnitude();
	_quat[0] /= m;
	_quat[1] /= m;
	_quat[2] /= m;
	_quat[3] /= m;
}

Quaternion Quaternion::operator * (const Quaternion & q) const
{
	Quaternion qr;
	qr._quat[0] = _quat[3] * q[0] + _quat[0] * q[3] + _quat[1] * q[2] - _quat[2] * q[1];
	qr._quat[1] = _quat[3] * q[1] - _quat[0] * q[2] + _quat[1] * q[3] + _quat[2] * q[0];
	qr._quat[2] = _quat[3] * q[2] + _quat[0] * q[1] - _quat[1] * q[0] + _quat[2] * q[3];
	qr._quat[3] = _quat[3] * q[3] - _quat[0] * q[0] - _quat[1] * q[1] - _quat[2] * q[2];
	qr.normalize();

	return qr;
}

Quaternion Quaternion::operator + (const Quaternion & q2) const
{
	Vector3 v1, v2;

	Vector3 t1(_quat[0], _quat[1], _quat[2]);
	v1 = t1;
	t1 *= q2[3];

	Vector3 t2(q2[0], q2[1], q2[2]);
	v2 = t2;
	t2 *= _quat[3];

	Vector3 t3, tf;
	t3 = t2.cross(t1);
	tf = t1 + t2;
	tf = t3 + tf;

	Quaternion dest;
	dest[0] = tf[0];
	dest[1] = tf[1];
	dest[2] = tf[2];
	dest[3] = _quat[3] * q2[3] - v1.dot(v2);

	return dest;
}

Quaternion & Quaternion::operator = (const Quaternion & q)
{
	_quat[0] = q._quat[0];
	_quat[1] = q._quat[1];
	_quat[2] = q._quat[2];
	_quat[3] = q._quat[3];
	return *this;
}

const Quaternion & Quaternion::operator *= (const Quaternion & q)
{
	*this = *this * q;
	return *this;
}

Matrix4 Quaternion::matrix(void) const
{
	real_t xx = _quat[0] * _quat[0];
	real_t xy = _quat[0] * _quat[1];
	real_t xz = _quat[0] * _quat[2];
	real_t xw = _quat[0] * _quat[3];
	real_t yy = _quat[1] * _quat[1];
	real_t yz = _quat[1] * _quat[2];
	real_t yw = _quat[1] * _quat[3];
	real_t zz = _quat[2] * _quat[2];
	real_t zw = _quat[2] * _quat[3];
	Matrix4 m;
	m.identity();
	m[0] = 1.0 - 2.0 * (yy + zz);
	m[1] = 2.0 * (xy + zw);
	m[2] = 2.0 * (xz - yw);
	m[3] = 0.0;
	m[4] = 2.0 * (xy - zw);
	m[5] = 1.0 - 2.0 * (xx + zz);
	m[6] = 2.0 * (yz + xw);
	m[7] = 0.0;
	m[8] = 2.0 * (xz + yw);
	m[9] = 2.0 * (yz - xw);
	m[10] = 1.0 - 2.0 * (xx + yy);
	m[11] = 0.0;
	m[12] = 0.0;
	m[13] = 0.0;
	m[14] = 0.0;
	m[15] = 1.0;
	return m;
}

// convert axis of rotation & rotation angle to quaternion
void Quaternion::rotation(const Vector3 & axis, real_t angle)
{
	if (axis.isZero())
	{
		_quat[0] = 0.0f;
		_quat[1] = 0.0f;
		_quat[2] = 0.0f;
		_quat[3] = 1.0f;
		return;
	}

	angle = deg2rad(angle); // phi
	Vector3 norm = axis;
	norm.normalize();
	real_t sin_a = sin(angle / 2.0);
	real_t cos_a = cos(angle / 2.0);
	_quat[0] = norm[0] * sin_a;
	_quat[1] = norm[1] * sin_a;
	_quat[2] = norm[2] * sin_a;
	_quat[3] = cos_a;
}

void Quaternion::axis(Vector3 & axis, real_t & angle) const
{
	real_t temp_angle, scale;

	temp_angle = acos(_quat[3]);
	scale = sqrt(_quat[0] * _quat[0] + _quat[1] * _quat[1] + _quat[2] * _quat[2]);

	assert(0.0 <= temp_angle);
	assert(PI >= temp_angle);

	if (float_eq(0.0f, scale))
	{
		angle = 0.0f;
		axis = Vector3(0.0f, 0.0f, 1.0f);          // any axis will do
	}
	else
	{
		angle = (temp_angle * 2.0);             // angle in radians
		axis = Vector3((_quat[0] / scale), (_quat[1] / scale), (_quat[2] / scale));
		axis.normalize();
		assert(0.0f <= angle);
		assert(2.*PI >= angle);
		assert(axis.isUnit());
		angle = rad2deg(angle);
	}
}

void Quaternion::euler(real_t ax, real_t ay, real_t az)
{
	/*
		Qx = [ cos(a/2), (sin(a/2), 0, 0)]
		Qy = [ cos(b/2), (0, sin(b/2), 0)]
		Qz = [ cos(c/2), (0, 0, sin(c/2))]
	*/
	ax = deg2rad(ax);
	ay = deg2rad(ay);
	az = deg2rad(az);
	Quaternion qx(sin(ax / 2.), 0., 0., cos(ax / 2.));
	Quaternion qy(0., sin(ay / 2.), 0., cos(ay / 2.));
	Quaternion qz(0., 0., sin(az / 2.), cos(ax / 2.));
	*this = qx * qy * qz;
	normalize();
}

Vector3 Quaternion::euler(void) const
{
	Matrix4 mat = matrix();
	real_t C, D, rx, ry, angle_x, angle_y, angle_z;
	angle_y = D = asin(mat[2]); // calculate y axis angle
	C = cos(angle_y);
	angle_y = rad2deg(angle_y);
	if (fabs(C) > 0.005) // avoid gimbal lock
	{
		rx = mat[10] / C; // get x axis angle
		ry = -mat[6] / C;
		angle_x = rad2deg(atan2(ry, rx));
		rx = mat[0] / C; // get z axis angle
		rx = -mat[1] / C;
		angle_z = rad2deg(atan2(ry, rx));
	}
	else // gimbal lock
	{
		angle_x = 0.;
		rx = mat[5];
		ry = mat[4];
		angle_z = rad2deg(atan2(ry, rx));
	}
	// return only positive angles in [0,360]
	if (angle_x < 0.)
		angle_x += 360.;
	if (angle_y < 0.)
		angle_y += 360.;
	if (angle_z < 0.)
		angle_z += 360.;
	Vector3 v(angle_x, angle_y, angle_z);
	return v;
}

real_t Quaternion::operator[](unsigned int i) const
{
	assert (i < 4);
	return _quat[i];
}

real_t & Quaternion::operator[](unsigned int i)
{
	assert (i < 4);
	return _quat[i];
}

std::string Quaternion::str(unsigned int i) const
{
	assert (i < 4);
	std::stringstream buf;
	buf << _quat[i];
	return buf.str();
}
