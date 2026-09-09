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
};
