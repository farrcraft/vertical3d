/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <glm/glm.hpp>

namespace v3d::brep {

    class Vertex final {
     public:
        Vertex();
        Vertex(const glm::vec3 & p);
        ~Vertex();

        // const, because C++20's reversed candidate for a non-const operator== makes every
        // a == b ambiguous with the b == a it synthesizes
        bool operator == (const Vertex & v) const;
        bool operator == (const glm::vec3& v) const;

        /**
         * Whether this component is selected. Selection is per component, not per mesh, so
         * a vertex, an edge and a face each carry their own.
         **/
        bool selected(void) const noexcept;
        void selected(bool sel) noexcept;

        unsigned int edge(void) const;
        void edge(unsigned int e);
        glm::vec3 point(void) const;
        void point(const glm::vec3 & p);

     private:
        glm::vec3 point_;
        unsigned int edge_;
        bool selected_;
    };

};  // namespace v3d::brep
