/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "MortonCode.h"

// http://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
// "Insert" a 0 bit after each of the 16 low bits of x
unsigned int Part1By1(unsigned int x) {
  x &= 0x0000ffff;                   // x = ---- ---- ---- ---- fedc ba98 7654 3210
  x = (x ^ (x <<  8)) & 0x00ff00ff;  // x = ---- ---- fedc ba98 ---- ---- 7654 3210
  x = (x ^ (x <<  4)) & 0x0f0f0f0f;  // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
  x = (x ^ (x <<  2)) & 0x33333333;  // x = --fe --dc --ba --98 --76 --54 --32 --10
  x = (x ^ (x <<  1)) & 0x55555555;  // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
  return x;
}

// "Insert" two 0 bits after each of the 10 low bits of x
unsigned int Part1By2(unsigned int x) {
  x &= 0x000003ff;                   // x = ---- ---- ---- ---- ---- --98 7654 3210
  x = (x ^ (x << 16)) & 0xff0000ff;  // x = ---- --98 ---- ---- ---- ---- 7654 3210
  x = (x ^ (x <<  8)) & 0x0300f00f;  // x = ---- --98 ---- ---- 7654 ---- ---- 3210
  x = (x ^ (x <<  4)) & 0x030c30c3;  // x = ---- --98 ---- 76-- --54 ---- 32-- --10
  x = (x ^ (x <<  2)) & 0x09249249;  // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  return x;
}

unsigned int Compact1By1(unsigned int x) {
  x &= 0x55555555;                   // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
  x = (x ^ (x >>  1)) & 0x33333333;  // x = --fe --dc --ba --98 --76 --54 --32 --10
  x = (x ^ (x >>  2)) & 0x0f0f0f0f;  // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
  x = (x ^ (x >>  4)) & 0x00ff00ff;  // x = ---- ---- fedc ba98 ---- ---- 7654 3210
  x = (x ^ (x >>  8)) & 0x0000ffff;  // x = ---- ---- ---- ---- fedc ba98 7654 3210
  return x;
}

// Inverse of Part1By2 - "delete" all bits not at positions divisible by 3
unsigned int Compact1By2(unsigned int x) {
  x &= 0x09249249;                   // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  x = (x ^ (x >>  2)) & 0x030c30c3;  // x = ---- --98 ---- 76-- --54 ---- 32-- --10
  x = (x ^ (x >>  4)) & 0x0300f00f;  // x = ---- --98 ---- ---- 7654 ---- ---- 3210
  x = (x ^ (x >>  8)) & 0xff0000ff;  // x = ---- --98 ---- ---- ---- ---- 7654 3210
  x = (x ^ (x >> 16)) & 0x000003ff;  // x = ---- ---- ---- ---- ---- --98 7654 3210
  return x;
}

unsigned int DecodeMorton2X(unsigned int code) {
  return Compact1By1(code >> 0);
}

unsigned int DecodeMorton2Y(unsigned int code) {
  return Compact1By1(code >> 1);
}

unsigned int DecodeMorton3X(unsigned int code) {
  return Compact1By2(code >> 0);
}

unsigned int DecodeMorton3Y(unsigned int code) {
  return Compact1By2(code >> 1);
}

unsigned int DecodeMorton3Z(unsigned int code) {
  return Compact1By2(code >> 2);
}

unsigned int MortonCode::encode(const glm::ivec2 & vec) const {
  return (Part1By1(vec.y) << 1) + Part1By1(vec.x);
}

unsigned int MortonCode::encode(const glm::ivec3 & vec) const {
  return (Part1By2(vec.z) << 2) + (Part1By2(vec.y) << 1) + Part1By2(vec.x);
}

glm::ivec2 MortonCode::decode2(unsigned int code) const {
    glm::ivec2 vec;
    vec.x = DecodeMorton2X(code);
    vec.y = DecodeMorton2Y(code);
    return vec;
}

glm::ivec3 MortonCode::decode3(unsigned int code) const {
    glm::ivec3 vec;
    vec.x = DecodeMorton3X(code);
    vec.y = DecodeMorton3Y(code);
    vec.z = DecodeMorton3Z(code);
    return vec;
}
