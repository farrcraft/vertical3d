/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <glm/glm.hpp>

namespace v3d::brep {

	class Vertex final {
		public:
			Vertex();
			Vertex(const glm::vec3 & p);
			~Vertex();

			bool operator == (const Vertex & v);
			bool operator == (const glm::vec3& v);

			unsigned int edge(void) const;
			void edge(unsigned int e);
			glm::vec3 point(void) const;
			void point(const glm::vec3 & p);

		private:
			glm::vec3 point_;
			unsigned int edge_;
	};

};
