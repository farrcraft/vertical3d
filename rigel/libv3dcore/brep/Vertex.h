/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <libv3dtypes/Vector3.h>

namespace v3D
{

	class Vertex
	{
		public:
			Vertex();
			Vertex(const Vector3 & p);
			~Vertex();

			bool operator == (const Vertex & v);
			bool operator == (const Vector3 & v);

			bool selected(void) const;
			void selected(bool sel);

			unsigned int edge(void) const;
			void edge(unsigned int e);
			Vector3 point(void) const;
			void point(const Vector3 & p);

		private:
			Vector3				_point;
			unsigned int		_edgeID;
			bool				_selected;
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_VERTEX_H
