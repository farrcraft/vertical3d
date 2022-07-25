/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/glm.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>

namespace v3d::gl {

	/**
	 * A class for GPU buffer data
	 */
	class VertexBuffer
	{
		public:
			typedef enum
			{
				ATTRIBUTE_TYPE_COLOR,
				ATTRIBUTE_TYPE_NORMAL,
				ATTRIBUTE_TYPE_VERTEX,
				ATTRIBUTE_TYPE_TEXTURE,
				ATTRIBUTE_TYPE_GENERIC
			} AttributeType;

			typedef struct
			{
				AttributeType type_;
				unsigned int position_;
				unsigned int size_; // components in the attribute - 1-4
				unsigned int stride_;
				size_t length_;
				size_t offset_;
			} VertexAttribute;

			typedef enum
			{
				BUFFER_TYPE_STATIC,
				BUFFER_TYPE_DYNAMIC
			} BufferType;

			VertexBuffer(BufferType type);
			~VertexBuffer();

			/**
			 * Define a vertex buffer attribute
			 *
			 * @param unsigned int position attribute position
			 * @param unsigned int size the number of fields in each attribute element
			 * @param AttributeType type the type of data stored in this attribute
			 * @param unsigned int length the number of elements stored in this attribute
			 */
			void attribute(unsigned int position, unsigned int size, AttributeType type, size_t length);

			/**
			 * Upload attribute data
			 * 
			 * @param unsigned int attr the attribute position
			 * @param vector an array of attribute element data
			 */
			void data1f(unsigned int attr, const std::vector<float> & data);
			void data2f(unsigned int attr, const std::vector<glm::vec2> & data);
			void data3f(unsigned int attr, const std::vector<glm::vec3> & data);
			void data4f(unsigned int attr, const std::vector<glm::vec4> & data);

			void render();

			void indices(const std::vector<size_t> & data);
			void indices(glm::ivec3 * data, size_t count);
			void allocate();

			void set(unsigned int attr, const float * data, size_t size);

			bool allocated() const;

		private:
			unsigned int vao_;
			unsigned int ebo_;
			unsigned int vbo_;
			std::vector<VertexAttribute> attributes_;
			size_t indices_;
			BufferType type_;
			bool allocated_;
	};

};
