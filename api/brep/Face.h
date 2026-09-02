/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <glm/glm.hpp>

namespace v3d::brep {

    class Face final {
     public:
        Face();
        Face(const glm::vec3 & normal, unsigned int edge);
        ~Face();

        /**
         * Whether this component is selected. Selection is per component, not per mesh, so
         * a vertex, an edge and a face each carry their own.
         **/
        bool selected(void) const noexcept;
        void selected(bool sel) noexcept;

        glm::vec3 normal(void) const;
        void normal(const glm::vec3 & n);
        unsigned int edge(void) const;
        void edge(unsigned int e);

     private:
        glm::vec3 normal_;
        unsigned int edge_;
        bool selected_;
    };

};  // namespace v3d::brep
