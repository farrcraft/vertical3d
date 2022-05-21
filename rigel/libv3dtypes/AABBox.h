/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Vector3.h"
//#include "Polygon.h"

namespace v3D
{
	/**
		A class defining an Axis Aligned Bounding Box.
	 */
	class AABBox
	{
		public:
			AABBox();
			~AABBox();

			/**
				Get the minimum extents of the bounding box.

				@return a vector of the minimum extents of the box.
			 */
			Vector3		min(void) const;
			/**
				Get the maximum extents of the bounding box.

				@return a vector of the maximum extents of the box.
			 */
			Vector3		max(void) const;
			/**
				Get the 8 vertices that compose the bounding box

				@param verts an array of Vector3's to hold the 8 vertices.
			 */
			void	 	vertices(Vector3 * verts) const;
			/**
				Set the minimum and maximum extents of the bounding box.

				@param min the minimum extents of the bounding box.
				@param max the maximum extents of the bounding box.
			*/
			void		extents(const Vector3 & min, const Vector3 & max);
			/**
				Get the origin of the bounding box.

				@return the origin of the bounding box.
			*/
			Vector3		origin(void) const;

			/**
				Extend bounds to include a point.
				If the point is already within the bounding box then there is no
				change to the bounding box. The point and bounding box are 
				assumed to exist in the same coordinate space.

				@param point the point to include in the bounding box.
			 */
			void		extend(const Vector3 & point);

			// set min & max to bound polygon
//			void		bound(const Polygon & poly);

		private:
			Vector3		_min;
			Vector3		_max;
	};
	
}; // end namespace v3D
