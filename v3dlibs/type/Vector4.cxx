/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "Vector4.h"
#include "3dtypes.h"

#include <cmath>
#include <cassert>
#include <sstream>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace v3d::type;

Vector4::Vector4() {
	vec_[0] = vec_[1] = vec_[2] = vec_[3] = 0.0f;
}

Vector4::Vector4(float val) {
	vec_[0] = vec_[1] = vec_[2] = vec_[3] = val;
}

Vector4::Vector4(float x, float y, float z, float w) {
	vec_[0] = x;
	vec_[1] = y;
	vec_[2] = z;
	vec_[3] = w;
}

Vector4::Vector4(const Vector3 & vec) {
	vec_[0] = vec[0];
	vec_[1] = vec[1];
	vec_[2] = vec[2];
	vec_[3] = 0.0f;
}

Vector4::Vector4(const Vector3 & vec, float w) {
	vec_[0] = vec[0];
	vec_[1] = vec[1];
	vec_[2] = vec[2];
	vec_[3] = w;
}

Vector4::Vector4(const std::string & val) {
	vec_[0] = vec_[1] = vec_[2] = vec_[3] = 0.0f;
	if (!val.empty()) {
		std::string var_x, var_y, var_z, var_w;
		size_t pos, pos2, pos3;
		pos = val.find(",");
		pos2 = val.find(",", pos+1);
		pos3 = val.find(",", pos2+1);
		var_x = val.substr(0, pos);
		boost::trim(var_x);
		var_y = val.substr(pos+1, pos2 - pos - 1);
		boost::trim(var_y);
		var_z = val.substr(pos2+1, pos3 - pos2 - 1);
		boost::trim(var_z);
		var_w = val.substr(pos3+1, val.length() - pos3);
		boost::trim(var_w);
		try {
			if (!var_x.empty()) {
				vec_[0] = boost::lexical_cast<float>(var_x);
			}
			if (!var_y.empty()) {
				vec_[1] = boost::lexical_cast<float>(var_y);
			}
			if (!var_z.empty()) {
				vec_[2] = boost::lexical_cast<float>(var_z);
			}
			if (!var_w.empty()) {
				vec_[3] = boost::lexical_cast<float>(var_w);
			}
		}
		catch (boost::bad_lexical_cast &) {
		}
	}
}

bool Vector4::operator == (const Vector4 & v) const {
	return  ((vec_[0] == v.vec_[0]) && 
			 (vec_[1] == v.vec_[1]) && 
			 (vec_[2] == v.vec_[2]) &&
			 (vec_[3] == v.vec_[3])
			 ) ? true : false;
}

bool Vector4::operator != (const Vector4 &v) const {
	return !(*this == v);
}

Vector4 & Vector4::operator += (const Vector4 & v) {
	vec_[0] += v.vec_[0];
	vec_[1] += v.vec_[1];
	vec_[2] += v.vec_[2];
	vec_[3] += v.vec_[3];

	return *this;
}

Vector4 & Vector4::operator *= (float f) {
	vec_[0] *= f;
	vec_[1] *= f;
	vec_[2] *= f;
	vec_[3] *= f;

	return *this;
}

Vector4 & Vector4::operator /= (const float f) {
	assert (f != 0.0f);

	float inv = 1.0f / f;

	vec_[0] *= inv;
	vec_[1] *= inv;
	vec_[2] *= inv;
	vec_[3] *= inv;

	return *this;
}

Vector4 Vector4::operator - () const {
	return Vector4(-vec_[0], -vec_[1], -vec_[2], -vec_[3]);
}

float Vector4::operator[](unsigned int i) const {
	assert (i < 4);
	return vec_[i];
}

float & Vector4::operator[](unsigned int i) {
	assert (i < 4);
	return vec_[i];
}

void Vector4::reset(void) {
	vec_[0] = vec_[1] = vec_[2] = vec_[3] = 0.0f;
}

// also known as an inner product
float Vector4::dot(const Vector4 & other) const {
	return ((vec_[0] * other.vec_[0]) + (vec_[1] * other.vec_[1]) + (vec_[2] * other.vec_[2]) + (vec_[3] * other.vec_[3]));
}

float Vector4::squaredLength() const {
	return (vec_[0] * vec_[0] + vec_[1] * vec_[1] + vec_[2] * vec_[2] + vec_[3] * vec_[3]);
}

float Vector4::length() const {
	return static_cast<float>(std::sqrt(squaredLength()));
}

void Vector4::normalize() {
	float r = length();

	if (fabs(r - 1.0f) < EPSILON)
		return;

	r = 1.0f / r;

	*this *= r;
}

bool Vector4::unit() const {
	return float_eq(1.0f, length());
}

bool Vector4::zero() const {
	return float_eq(0.0f, length());
}

const float * Vector4::operator*() const {
	const float * f = vec_;
	return f;
}

std::string Vector4::str() const {
	std::stringstream stream;
	stream << vec_[0] << ", " << vec_[1] << ", " << vec_[2] << ", " << vec_[3];
	std::string str = stream.str();
	return str;
}

const Vector4 v3d::types::operator + (const Vector4 & lhs, const Vector4 & rhs) {
	return Vector4(lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2], lhs[3] + rhs[3]);
}

const Vector4 v3d::types::operator + (const Vector4 & lhs, const float f) {
	return Vector4(lhs[0] + f, lhs[1] + f, lhs[2] + f, lhs[3] + f);
}

const Vector4 v3d::types::operator - (const Vector4 & lhs, const Vector4 & rhs) {
	return Vector4(lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2], lhs[3] - rhs[3]);
}

const float v3d::types::operator * (const Vector4 & lhs, const Vector4 & rhs) {
	return lhs.dot(rhs);
}

const Vector4 v3d::types::operator / (const Vector4 & lhs, const float f) {
	assert (f != 0.0f);
	float inv = 1.0f / f;
	return Vector4(lhs[0] * inv, lhs[1] * inv, lhs[2] * inv, lhs[3] * inv);
}

const Vector4 v3d::types::operator * (const Vector4 & lhs, float f) {
	Vector4 vec;
	vec[0] = lhs[0] * f;
	vec[1] = lhs[1] * f;
	vec[2] = lhs[2] * f;
	vec[3] = lhs[3] * f;

	return vec;
}
