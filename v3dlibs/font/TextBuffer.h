/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace v3d::font {
	/**
	 * A base text buffer class that can be inherited by specific text types
	 */
	class TextBuffer {
		public:

			/**
			 * Clear the existing text buffer
			 */
			void clear();
			void invalidate();
			bool dirty() const;

			void resize(unsigned int size);

			std::vector<glm::vec3> & vertices();
			std::vector<glm::vec2> & uvs();
			std::vector<glm::vec4> & colors();
			std::vector<unsigned int> & indices();

			void addVertex(const glm::vec3 & vertex);
			void addIndex(unsigned int index);
			void addColor(const glm::vec4 & color);
			void addTextureCoordinate(const glm::vec2 & uv);
			void dirty(bool state);

		private:
			bool dirty_;

			std::vector<glm::vec3> vertices_;
			std::vector<glm::vec2> uvs_;
			std::vector<glm::vec4> colors_;
			std::vector<unsigned int> indices_;
	};
};