/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

/**
 * This is a collection of common constant mathematical values (e.g. PI, EPSILON) and assorted useful methods.
 * 
 * 
 **/
// clamp(var, min, max)
template<class T>
inline T clamp(T v, T l, T h) { return ((v) < (l) ? (l) : (v) > (h) ? (h) : v); }

const float PI =  3.14159265358979323846f;	// Pi

template<class T> 
inline T rad2deg(T x) { return (((x) * 180.0f) / PI); }

template<class T> 
inline T deg2rad(T x) { return (((x) * PI) / 180.0f); }

const float EPSILON		= 1.0E-10f;

template<class T> inline bool float_eq(T x, T v) { return ( ((v) - EPSILON) < (x) && (x) < ((v) + EPSILON) ); }

template<class T> inline bool is_zero(T f) { return (f < EPSILON && f > -EPSILON); }

template<class T> inline void swap(T & a, T & b) { T c = a; a = b; b = c; }

inline int floor_log2(unsigned int n)
{
	int pos = 0;
	if (n >= 1<<16) { n >>= 16; pos += 16; }
	if (n >= 1<< 8) { n >>=  8; pos +=  8; }
	if (n >= 1<< 4) { n >>=  4; pos +=  4; }
	if (n >= 1<< 2) { n >>=  2; pos +=  2; }
	if (n >= 1<< 1) {           pos +=  1; }
	return ((n == 0) ? (-1) : pos);
}

inline unsigned int npot(unsigned int n)
{
	--n;
	n |= n >> 16;
	n |= n >> 8;
	n |= n >> 4;
	n |= n >> 2;
	n |= n >> 1;
	++n;
	return n;
}

#define RANDOM_FLOAT (static_cast<float>(rand())/static_cast<float>(RAND_MAX))
