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

		private:
			glm::vec3 point_;
			glm::vec3 color_;
			glm::vec3 normal_;
			unsigned int bits_;
	};
};
