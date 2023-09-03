/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <deque>
#include <vector>

#include "VertexBuffer.h"

#include <glm/glm.hpp>

namespace v3d::gl {
    /**
     * An OpenGL Canvas for rendering 2D primitives
     */
    class Canvas {
     public:
        Canvas();
        Canvas(unsigned int width, unsigned int height);
        ~Canvas();

        /**
            * Upload canvas primitive data to the GPU
            */
        void upload();

        /**
            * Clear the canvas
            */
        void clear();
        /**
            * Update the size of the canvas
            */
        void resize(unsigned int width, unsigned int height);

        const VertexBuffer& buffer() const;

        void rect(unsigned int left, unsigned int right, unsigned int top, unsigned int bottom, glm::vec3 color);
        void circle(size_t sides, size_t size, glm::vec3 color);
        /**
            * Push an identity matrix onto the modelview matrix stack
            */
        void push();
        /**
            * Pop the top matrix off the modelview matrix stack
            */
        void pop();
        /**
            * Translate the current modelview matrix
            */
        void translate(glm::vec2 pos);

     protected:
        void addVertex(const glm::vec3 & position, const glm::vec4 & color);
        void addQuad(const glm::vec2 & xy0, const glm::vec2 & xy1, const glm::vec4 & color);

     private:
        unsigned int width_;
        unsigned int height_;
        std::deque<glm::mat4> modelView_;
        VertexBuffer buffer_;
        unsigned int vao_;

        // vertex data
        std::vector<glm::vec3> xyz_;
        std::vector<glm::vec4> rgba_;
        std::vector<size_t> indices_;
    };

};  // namespace v3d::gl
