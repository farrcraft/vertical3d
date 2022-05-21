/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <libv3dtypes/Vector3.h>

namespace v3D
{

	class Face
	{
		public:
			Face();
			Face(const Vector3 & normal, unsigned int edgeID);
			~Face();

			bool selected(void) const;
			void selected(bool sel);

			Vector3 normal(void) const;
			void normal(const Vector3 & n);
			unsigned int edge(void) const;
			void edge(unsigned int e);

		private:
			Vector3			_normal;
			unsigned int	_edgeID;
			bool			_selected;
	};

}; // end namespace v3D
