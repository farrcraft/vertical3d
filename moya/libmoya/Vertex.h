/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/glm.hpp>

namespace v3d::moya {
    class Vertex {
     public:
        Vertex();
        ~Vertex();

        enum Bits {
            HAS_COLOR = (1 << 1),
            HAS_NORMAL = (1 << 2),
            HAS_TEX_COORD = (1 << 3)
        };

        glm::vec3 point(void) const;
        void point(const glm::vec3 & v);

        /**
         * The shaded colour. Set on every vertex of a grid rather than per micropolygon,
         * since a micropolygon shares its corners with its neighbours.
         */
        glm::vec3 color(void) const;
        void color(const glm::vec3 & c);
        /**
         * Whether a colour was written. A primitive carrying no "Cs" takes the colour that
         * was current when it was added, and one that carries its own keeps it.
         */
        bool hasColor(void) const;

     private:
        // a grid is filled in by writing over default constructed vertices, so a vertex that
        // has not been written to yet has to read as one rather than as whatever was there
        glm::vec3 point_ = glm::vec3(0.0f);
        glm::vec3 color_ = glm::vec3(0.0f);
        glm::vec3 normal_ = glm::vec3(0.0f);
        unsigned int bits_ = 0;
    };
};  // namespace v3d::moya
