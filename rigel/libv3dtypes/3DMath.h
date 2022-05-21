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
#ifndef INCLUDED_MOYA_3DMATH_H
#define INCLUDED_MOYA_3DMATH_H

namespace v3D
{

	typedef float real_t;

	const float PI      	=  3.14159265358979323846f;	// Pi
	const float EPSILON 	= 0.005f;					// error tolerance for check
	const float gEpsilon	= 1.0E-10;

	template<class T> inline bool float_eq(T x, T v) { return ( ((v) - EPSILON) < (x) && (x) < ((v) + EPSILON) ); }
	template<class T> inline T rad2deg(T x) { return (((x) * 180.0) / PI); }
	template<class T> inline T deg2rad(T x) { return (((x) * PI) / 180.0); }
	template<class T> inline T vclamp(T v, T l, T h) { return ((v) < (l) ? (l) : (v) > (h) ? (h) : v); }
	template<class T> inline T min(T a, T b) { return (a < b) ? a : b; }
	template<class T> inline T max(T a, T b) { return (a > b) ? a : b; }
	template<class T> inline T min(T a, T b, T c) { return (a < b) ? (a < c ? a : c) : b; }
	template<class T> inline T max(T a, T b, T c) { return (a > b) ? (a > c ? a : c) : b; }
	template<class T> inline bool isZero(T f) { return (f < gEpsilon && f > -gEpsilon); }
	template<class T> inline void swap(T & a, T & b) { T c = a; a = b; b = c; }

}; // end namespace v3D

#endif // INCLUDED_MOYA_3DMATH_H
