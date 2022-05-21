/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "../3dtypes/Vector3.h"

namespace v3d::brep
{

	class Face
	{
		public:
			Face();
			Face(const v3d::types::Vector3 & normal, unsigned int edge);
			~Face();

			/*
			bool selected(void) const;
			void selected(bool sel);
			*/

			v3d::types::Vector3 normal(void) const;
			void normal(const v3d::types::Vector3 & n);
			unsigned int edge(void) const;
			void edge(unsigned int e);

		private:
			v3d::types::Vector3	normal_;
			unsigned int edge_;
	};

};
