/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#pragma once

#include <glm/glm.hpp>


class MortonCode {
 public:
    static unsigned int encode(const glm::ivec2 & vec);
    static unsigned int encode(const glm::ivec3 & vec);

    static glm::ivec2 decode2(unsigned int code);
    static glm::ivec3 decode3(unsigned int code);

 private:
    // The masks that spread the 16 low bits of a word to every second bit. The first keeps
    // the input bits, and each other is applied after the shift it is named for. Compacting
    // applies them in the opposite order.
    static constexpr unsigned int SPREAD2_INPUT = 0x0000ffff;
    static constexpr unsigned int SPREAD2_SHIFT8 = 0x00ff00ff;
    static constexpr unsigned int SPREAD2_SHIFT4 = 0x0f0f0f0f;
    static constexpr unsigned int SPREAD2_SHIFT2 = 0x33333333;
    static constexpr unsigned int SPREAD2_SHIFT1 = 0x55555555;

    // The same for the 10 low bits of a word, spread to every third bit.
    static constexpr unsigned int SPREAD3_INPUT = 0x000003ff;
    static constexpr unsigned int SPREAD3_SHIFT16 = 0xff0000ff;
    static constexpr unsigned int SPREAD3_SHIFT8 = 0x0300f00f;
    static constexpr unsigned int SPREAD3_SHIFT4 = 0x030c30c3;
    static constexpr unsigned int SPREAD3_SHIFT2 = 0x09249249;

    // http://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
    // "Insert" a 0 bit after each of the 16 low bits of x
    static unsigned int Part1By1(unsigned int x);

    // "Insert" two 0 bits after each of the 10 low bits of x
    static unsigned int Part1By2(unsigned int x);

    static unsigned int Compact1By1(unsigned int x);

    // Inverse of Part1By2 - "delete" all bits not at positions divisible by 3
    static unsigned int Compact1By2(unsigned int x);

    static unsigned int DecodeMorton2X(unsigned int code);
    static unsigned int DecodeMorton2Y(unsigned int code);
    static unsigned int DecodeMorton3X(unsigned int code);
    static unsigned int DecodeMorton3Y(unsigned int code);
    static unsigned int DecodeMorton3Z(unsigned int code);
};
