/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#include "VertexBuffer.h"

#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>

namespace v3d::gl {

    VertexBuffer::VertexBuffer(BufferType type) :
        ebo_(0),
        vbo_(0),
        type_(type),
        allocated_(false)
    {
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
    }

    VertexBuffer::~VertexBuffer()
    {
        if (ebo_ != 0)
        {
            glDeleteBuffers(1, &ebo_);
        }
        glDeleteBuffers(1, &vbo_);
    }

    void VertexBuffer::attribute(unsigned int position, unsigned int size, AttributeType type, size_t length)
    {
        VertexAttribute attr;
        attr.position_ = position;
        attr.size_ = size;
        attr.type_ = type;
        attr.length_ = length;
        attr.stride_ = sizeof(float) * size;

        size_t offset = 0;
        bool exists = false;
        unsigned int index;
        for (unsigned int i = 0; i < attributes_.size(); i++)
        {
            if (attributes_[i].position_ < position)
            {
                offset += attributes_[i].stride_ * attributes_[i].length_;
            }
            if (attributes_[i].position_ == position)
            {
                exists = true;
                index = i;
            }
        }
        attr.offset_ = offset;

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(
            position,
            attr.size_,
            GL_FLOAT,
            GL_FALSE,
            attr.stride_,
            reinterpret_cast<void*>(attr.offset_)
        );

        // if the attribute in the same poisition has previously been set then just replace the old version
        if (exists)
        {
            attributes_[index] = attr;
        }
        else
        {
            attributes_.push_back(attr);
        }
    }

    void VertexBuffer::indices(const std::vector<size_t>& data) {
        indices_ = data.size();
        if (ebo_ == 0) {
            glGenBuffers(1, &ebo_);
        }
        GLenum usage;
        switch (type_)
        {
        case BUFFER_TYPE_STATIC:
            usage = GL_STATIC_DRAW;
            break;
        case BUFFER_TYPE_DYNAMIC:
            usage = GL_DYNAMIC_DRAW;
            break;
        default:
            usage = GL_STATIC_DRAW;
            break;
        }
        glBindVertexArray(vao_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_ * sizeof(unsigned int), &data[0], usage);
    }

    void VertexBuffer::indices(glm::ivec3* data, size_t count) {
        indices_ = count;
        if (ebo_ == 0) {
            glGenBuffers(1, &ebo_);
        }
        GLenum usage;
        switch (type_) {
        case BUFFER_TYPE_STATIC:
            usage = GL_STATIC_DRAW;
            break;
        case BUFFER_TYPE_DYNAMIC:
            usage = GL_DYNAMIC_DRAW;
            break;
        default:
            usage = GL_STATIC_DRAW;
            break;
        }
        glBindVertexArray(vao_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_ * sizeof(unsigned int), &(data[0].x), usage);
    }


    void VertexBuffer::allocate() {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);

        size_t bufferSize = 0;
        for (unsigned int i = 0; i < attributes_.size(); i++) {
            bufferSize += attributes_[i].stride_ * attributes_[i].length_;
        }

        GLenum usage;
        switch (type_) {
        case BUFFER_TYPE_STATIC:
            usage = GL_STATIC_DRAW;
            break;
        case BUFFER_TYPE_DYNAMIC:
            usage = GL_DYNAMIC_DRAW;
            break;
        default:
            usage = GL_STATIC_DRAW;
            break;
        }

        // allocate buffer
        glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, usage);
        allocated_ = true;
    }

    bool VertexBuffer::allocated() const
    {
        return allocated_;
    }

    void VertexBuffer::data1f(unsigned int attr, const std::vector<float>& data)
    {
        set(attr, &data[0], data.size());
    }

    void VertexBuffer::data2f(unsigned int attr, const std::vector<glm::vec2>& data)
    {
        set(attr, glm::value_ptr(data[0]), data.size());
    }

    void VertexBuffer::data3f(unsigned int attr, const std::vector<glm::vec3>& data)
    {
        set(attr, glm::value_ptr(data[0]), data.size());
    }

    void VertexBuffer::data4f(unsigned int attr, const std::vector<glm::vec4>& data)
    {
        set(attr, glm::value_ptr(data[0]), data.size());
    }

    void VertexBuffer::set(unsigned int attr, const float* data, size_t size)
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        size_t offset = attributes_[attr].offset_;
        size_t byteSize = attributes_[attr].stride_ * size;
        glBufferSubData(GL_ARRAY_BUFFER, offset, byteSize, data);
    }

    void VertexBuffer::render() const
    {
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_), GL_UNSIGNED_INT, reinterpret_cast<void*>(0));
    }

};  // namespace v3d::gl
