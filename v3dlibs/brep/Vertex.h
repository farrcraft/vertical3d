/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "../3dtypes/Vector3.h"

namespace v3d::brep
{

	class Vertex
	{
		public:
			Vertex();
			Vertex(const v3d::types::Vector3 & p);
			~Vertex();

			bool operator == (const Vertex & v);
			bool operator == (const v3d::types::Vector3 & v);

			unsigned int edge(void) const;
			void edge(unsigned int e);
			v3d::types::Vector3 point(void) const;
			void point(const v3d::types::Vector3 & p);

		private:
			v3d::types::Vector3	point_;
			unsigned int edge_;
	};

};
