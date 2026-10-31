/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "MortonCode.h"

unsigned int MortonCode::Part1By1(unsigned int x) {
  x &= SPREAD2_INPUT;                      // x = ---- ---- ---- ---- fedc ba98 7654 3210
  x = (x ^ (x <<  8)) & SPREAD2_SHIFT8;    // x = ---- ---- fedc ba98 ---- ---- 7654 3210
  x = (x ^ (x <<  4)) & SPREAD2_SHIFT4;    // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
  x = (x ^ (x <<  2)) & SPREAD2_SHIFT2;    // x = --fe --dc --ba --98 --76 --54 --32 --10
  x = (x ^ (x <<  1)) & SPREAD2_SHIFT1;    // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
  return x;
}

unsigned int MortonCode::Part1By2(unsigned int x) {
  x &= SPREAD3_INPUT;                      // x = ---- ---- ---- ---- ---- --98 7654 3210
  x = (x ^ (x << 16)) & SPREAD3_SHIFT16;   // x = ---- --98 ---- ---- ---- ---- 7654 3210
  x = (x ^ (x <<  8)) & SPREAD3_SHIFT8;    // x = ---- --98 ---- ---- 7654 ---- ---- 3210
  x = (x ^ (x <<  4)) & SPREAD3_SHIFT4;    // x = ---- --98 ---- 76-- --54 ---- 32-- --10
  x = (x ^ (x <<  2)) & SPREAD3_SHIFT2;    // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  return x;
}

unsigned int MortonCode::Compact1By1(unsigned int x) {
  x &= SPREAD2_SHIFT1;                     // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
  x = (x ^ (x >>  1)) & SPREAD2_SHIFT2;    // x = --fe --dc --ba --98 --76 --54 --32 --10
  x = (x ^ (x >>  2)) & SPREAD2_SHIFT4;    // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
  x = (x ^ (x >>  4)) & SPREAD2_SHIFT8;    // x = ---- ---- fedc ba98 ---- ---- 7654 3210
  x = (x ^ (x >>  8)) & SPREAD2_INPUT;     // x = ---- ---- ---- ---- fedc ba98 7654 3210
  return x;
}

unsigned int MortonCode::Compact1By2(unsigned int x) {
  x &= SPREAD3_SHIFT2;                     // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  x = (x ^ (x >>  2)) & SPREAD3_SHIFT4;    // x = ---- --98 ---- 76-- --54 ---- 32-- --10
  x = (x ^ (x >>  4)) & SPREAD3_SHIFT8;    // x = ---- --98 ---- ---- 7654 ---- ---- 3210
  x = (x ^ (x >>  8)) & SPREAD3_SHIFT16;   // x = ---- --98 ---- ---- ---- ---- 7654 3210
  x = (x ^ (x >> 16)) & SPREAD3_INPUT;     // x = ---- ---- ---- ---- ---- --98 7654 3210
  return x;
}

unsigned int MortonCode::DecodeMorton2X(unsigned int code) {
  return Compact1By1(code >> 0);
}

unsigned int MortonCode::DecodeMorton2Y(unsigned int code) {
  return Compact1By1(code >> 1);
}

unsigned int MortonCode::DecodeMorton3X(unsigned int code) {
  return Compact1By2(code >> 0);
}

unsigned int MortonCode::DecodeMorton3Y(unsigned int code) {
  return Compact1By2(code >> 1);
}

unsigned int MortonCode::DecodeMorton3Z(unsigned int code) {
  return Compact1By2(code >> 2);
}

unsigned int MortonCode::encode(const glm::ivec2 & vec) {
  return (Part1By1(vec.y) << 1) + Part1By1(vec.x);
}

unsigned int MortonCode::encode(const glm::ivec3 & vec) {
  return (Part1By2(vec.z) << 2) + (Part1By2(vec.y) << 1) + Part1By2(vec.x);
}

glm::ivec2 MortonCode::decode2(unsigned int code) {
    glm::ivec2 vec;
    vec.x = static_cast<int>(DecodeMorton2X(code));
    vec.y = static_cast<int>(DecodeMorton2Y(code));
    return vec;
}

glm::ivec3 MortonCode::decode3(unsigned int code) {
    glm::ivec3 vec;
    vec.x = static_cast<int>(DecodeMorton3X(code));
    vec.y = static_cast<int>(DecodeMorton3Y(code));
    vec.z = static_cast<int>(DecodeMorton3Z(code));
    return vec;
}
