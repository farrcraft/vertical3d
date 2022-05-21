/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include <cassert>
#include <cmath>
#include <sstream>

#include "Quaternion.h"

using namespace v3d::type;

Quaternion::Quaternion()
{
	quat_[0] = 0.;
	quat_[1] = 0.;
	quat_[2] = 0.;
	quat_[3] = 1.;
}

Quaternion::Quaternion(const Quaternion & q)
{
	quat_[0] = q.quat_[0];
	quat_[1] = q.quat_[1];
	quat_[2] = q.quat_[2];
	quat_[3] = q.quat_[3];
}

Quaternion::Quaternion(float q[4])
{
	quat_[0] = q[0];
	quat_[1] = q[1];
	quat_[2] = q[2];
	quat_[3] = q[3];
}

Quaternion::Quaternion(float x, float y, float z, float w)
{
	quat_[0] = x;
	quat_[1] = y;
	quat_[2] = z;
	quat_[3] = w;
}

Quaternion::Quaternion(const Matrix4 & mat)
{
	float angle_x, angle_y, angle_z, tr_x, tr_y, C, D;

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
	Quaternion q(-quat_[0], -quat_[1], -quat_[2], quat_[3]);
	return q;
}

float Quaternion::magnitude(void) const
{
	return sqrt(quat_[3] * quat_[3] + quat_[0] * quat_[0] + quat_[1] * quat_[1] + quat_[2] * quat_[2]);
}

void Quaternion::normalize(void)
{
	float m = magnitude();
	quat_[0] /= m;
	quat_[1] /= m;
	quat_[2] /= m;
	quat_[3] /= m;
}

Quaternion Quaternion::operator * (const Quaternion & q) const
{
	Quaternion qr;
	qr.quat_[0] = quat_[3] * q[0] + quat_[0] * q[3] + quat_[1] * q[2] - quat_[2] * q[1];
	qr.quat_[1] = quat_[3] * q[1] - quat_[0] * q[2] + quat_[1] * q[3] + quat_[2] * q[0];
	qr.quat_[2] = quat_[3] * q[2] + quat_[0] * q[1] - quat_[1] * q[0] + quat_[2] * q[3];
	qr.quat_[3] = quat_[3] * q[3] - quat_[0] * q[0] - quat_[1] * q[1] - quat_[2] * q[2];
	qr.normalize();

	return qr;
}

Quaternion Quaternion::operator + (const Quaternion & q2) const
{
	Vector3 v1, v2;

	Vector3 t1(quat_[0], quat_[1], quat_[2]);
	v1 = t1;
	t1 *= q2[3];

	Vector3 t2(q2[0], q2[1], q2[2]);
	v2 = t2;
	t2 *= quat_[3];

	Vector3 t3, tf;
	t3 = t2.cross(t1);
	tf = t1 + t2;
	tf = t3 + tf;

	Quaternion dest;
	dest[0] = tf[0];
	dest[1] = tf[1];
	dest[2] = tf[2];
	dest[3] = quat_[3] * q2[3] - v1.dot(v2);

	return dest;
}

Quaternion & Quaternion::operator = (const Quaternion & q)
{
	quat_[0] = q.quat_[0];
	quat_[1] = q.quat_[1];
	quat_[2] = q.quat_[2];
	quat_[3] = q.quat_[3];
	return *this;
}

const Quaternion & Quaternion::operator *= (const Quaternion & q)
{
	*this = *this * q;
	return *this;
}

Matrix4 Quaternion::matrix(void) const
{
	float xx = quat_[0] * quat_[0];
	float xy = quat_[0] * quat_[1];
	float xz = quat_[0] * quat_[2];
	float xw = quat_[0] * quat_[3];
	float yy = quat_[1] * quat_[1];
	float yz = quat_[1] * quat_[2];
	float yw = quat_[1] * quat_[3];
	float zz = quat_[2] * quat_[2];
	float zw = quat_[2] * quat_[3];
	Matrix4 m;
	m.identity();
	m[0] = 1.0f - 2.0f * (yy + zz);
	m[1] = 2.0f * (xy + zw);
	m[2] = 2.0f * (xz - yw);
	m[3] = 0.0f;
	m[4] = 2.0f * (xy - zw);
	m[5] = 1.0f - 2.0f * (xx + zz);
	m[6] = 2.0f * (yz + xw);
	m[7] = 0.0f;
	m[8] = 2.0f * (xz + yw);
	m[9] = 2.0f * (yz - xw);
	m[10] = 1.0f - 2.0f * (xx + yy);
	m[11] = 0.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
	m[15] = 1.0f;
	return m;
}

// convert axis of rotation & rotation angle to quaternion
void Quaternion::rotation(const Vector3 & axis, float angle)
{
	if (axis.zero())
	{
		quat_[0] = 0.0f;
		quat_[1] = 0.0f;
		quat_[2] = 0.0f;
		quat_[3] = 1.0f;
		return;
	}

	angle = deg2rad(angle); // phi
	Vector3 norm = axis;
	norm.normalize();
	float sin_a = sin(angle / 2.0f);
	float cos_a = cos(angle / 2.0f);
	quat_[0] = norm[0] * sin_a;
	quat_[1] = norm[1] * sin_a;
	quat_[2] = norm[2] * sin_a;
	quat_[3] = cos_a;
}

void Quaternion::axis(Vector3 & axis, float & angle) const
{
	float temp_angle, scale;

	temp_angle = acos(quat_[3]);
	scale = sqrt(quat_[0] * quat_[0] + quat_[1] * quat_[1] + quat_[2] * quat_[2]);

	assert(0.0f <= temp_angle);
	assert(PI >= temp_angle);

	if (float_eq(0.0f, scale))
	{
		angle = 0.0f;
		axis = Vector3(0.0f, 0.0f, 1.0f);          // any axis will do
	}
	else
	{
		angle = (temp_angle * 2.0f);             // angle in radians
		axis = Vector3((quat_[0] / scale), (quat_[1] / scale), (quat_[2] / scale));
		axis.normalize();
		assert(0.0f <= angle);
		assert(2.0f * PI >= angle);
		assert(axis.unit());
		angle = rad2deg(angle);
	}
}

void Quaternion::euler(float ax, float ay, float az)
{
	/*
		Qx = [ cos(a/2), (sin(a/2), 0, 0)]
		Qy = [ cos(b/2), (0, sin(b/2), 0)]
		Qz = [ cos(c/2), (0, 0, sin(c/2))]
	*/
	ax = deg2rad(ax);
	ay = deg2rad(ay);
	az = deg2rad(az);
	Quaternion qx(sin(ax / 2.0f), 0.0f, 0.0f, cos(ax / 2.0f));
	Quaternion qy(0.0f, sin(ay / 2.0f), 0.0f, cos(ay / 2.0f));
	Quaternion qz(0.0f, 0.0f, sin(az / 2.0f), cos(ax / 2.0f));
	*this = qx * qy * qz;
	normalize();
}

Vector3 Quaternion::euler(void) const
{
	Matrix4 mat = matrix();
	float C, D, rx, ry, angle_x, angle_y, angle_z;
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
		angle_x = 0.0f;
		rx = mat[5];
		ry = mat[4];
		angle_z = rad2deg(atan2(ry, rx));
	}
	// return only positive angles in [0,360]
	if (angle_x < 0.0f)
		angle_x += 360.0f;
	if (angle_y < 0.0f)
		angle_y += 360.0f;
	if (angle_z < 0.0f)
		angle_z += 360.0f;
	Vector3 v(angle_x, angle_y, angle_z);
	return v;
}

float Quaternion::operator[](unsigned int i) const
{
	assert (i < 4);
	return quat_[i];
}

float & Quaternion::operator[](unsigned int i)
{
	assert (i < 4);
	return quat_[i];
}

std::string Quaternion::str(unsigned int i) const
{
	assert (i < 4);
	std::stringstream buf;
	buf << quat_[i];
	return buf.str();
}
