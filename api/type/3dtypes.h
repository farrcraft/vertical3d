/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

inline int floor_log2(unsigned int n) {
    int pos = 0;
    if (n >= 1<<16) { n >>= 16; pos += 16; }
    if (n >= 1<< 8) { n >>=  8; pos +=  8; }
    if (n >= 1<< 4) { n >>=  4; pos +=  4; }
    if (n >= 1<< 2) { n >>=  2; pos +=  2; }
    if (n >= 1<< 1) {           pos +=  1; }
    return ((n == 0) ? (-1) : pos);
}

inline unsigned int npot(unsigned int n) {
    --n;
    n |= n >> 16;
    n |= n >> 8;
    n |= n >> 4;
    n |= n >> 2;
    n |= n >> 1;
    ++n;
    return n;
}

#define RANDOM_FLOAT (static_cast<float>(rand_r())/static_cast<float>(RAND_MAX))
