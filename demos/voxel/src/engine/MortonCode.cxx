/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "MortonCode.h"

namespace {

// The masks that spread the 16 low bits of a word to every second bit. The first keeps
// the input bits, and each other is applied after the shift it is named for. Compacting
// applies them in the opposite order.
constexpr unsigned int SPREAD2_INPUT = 0x0000ffff;
constexpr unsigned int SPREAD2_SHIFT8 = 0x00ff00ff;
constexpr unsigned int SPREAD2_SHIFT4 = 0x0f0f0f0f;
constexpr unsigned int SPREAD2_SHIFT2 = 0x33333333;
constexpr unsigned int SPREAD2_SHIFT1 = 0x55555555;

// The same for the 10 low bits of a word, spread to every third bit.
constexpr unsigned int SPREAD3_INPUT = 0x000003ff;
constexpr unsigned int SPREAD3_SHIFT16 = 0xff0000ff;
constexpr unsigned int SPREAD3_SHIFT8 = 0x0300f00f;
constexpr unsigned int SPREAD3_SHIFT4 = 0x030c30c3;
constexpr unsigned int SPREAD3_SHIFT2 = 0x09249249;

// http://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
// "Insert" a 0 bit after each of the 16 low bits of x
unsigned int Part1By1(unsigned int x) {
  x &= SPREAD2_INPUT;                      // x = ---- ---- ---- ---- fedc ba98 7654 3210
  x = (x ^ (x <<  8)) & SPREAD2_SHIFT8;    // x = ---- ---- fedc ba98 ---- ---- 7654 3210
  x = (x ^ (x <<  4)) & SPREAD2_SHIFT4;    // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
  x = (x ^ (x <<  2)) & SPREAD2_SHIFT2;    // x = --fe --dc --ba --98 --76 --54 --32 --10
  x = (x ^ (x <<  1)) & SPREAD2_SHIFT1;    // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
  return x;
}

// "Insert" two 0 bits after each of the 10 low bits of x
unsigned int Part1By2(unsigned int x) {
  x &= SPREAD3_INPUT;                      // x = ---- ---- ---- ---- ---- --98 7654 3210
  x = (x ^ (x << 16)) & SPREAD3_SHIFT16;   // x = ---- --98 ---- ---- ---- ---- 7654 3210
  x = (x ^ (x <<  8)) & SPREAD3_SHIFT8;    // x = ---- --98 ---- ---- 7654 ---- ---- 3210
  x = (x ^ (x <<  4)) & SPREAD3_SHIFT4;    // x = ---- --98 ---- 76-- --54 ---- 32-- --10
  x = (x ^ (x <<  2)) & SPREAD3_SHIFT2;    // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  return x;
}

unsigned int Compact1By1(unsigned int x) {
  x &= SPREAD2_SHIFT1;                     // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
  x = (x ^ (x >>  1)) & SPREAD2_SHIFT2;    // x = --fe --dc --ba --98 --76 --54 --32 --10
  x = (x ^ (x >>  2)) & SPREAD2_SHIFT4;    // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
  x = (x ^ (x >>  4)) & SPREAD2_SHIFT8;    // x = ---- ---- fedc ba98 ---- ---- 7654 3210
  x = (x ^ (x >>  8)) & SPREAD2_INPUT;     // x = ---- ---- ---- ---- fedc ba98 7654 3210
  return x;
}

// Inverse of Part1By2 - "delete" all bits not at positions divisible by 3
unsigned int Compact1By2(unsigned int x) {
  x &= SPREAD3_SHIFT2;                     // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  x = (x ^ (x >>  2)) & SPREAD3_SHIFT4;    // x = ---- --98 ---- 76-- --54 ---- 32-- --10
  x = (x ^ (x >>  4)) & SPREAD3_SHIFT8;    // x = ---- --98 ---- ---- 7654 ---- ---- 3210
  x = (x ^ (x >>  8)) & SPREAD3_SHIFT16;   // x = ---- --98 ---- ---- ---- ---- 7654 3210
  x = (x ^ (x >> 16)) & SPREAD3_INPUT;     // x = ---- ---- ---- ---- ---- --98 7654 3210
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

};  // namespace

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
