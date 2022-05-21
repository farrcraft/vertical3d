/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Vector3.h"
//#include "Polygon.h"

namespace v3d::type
{
	/**
	 * A class defining a 3D Axis Aligned Bounding Box.
	 */
	class AABBox
	{
		public:
			AABBox();
			~AABBox();

			/**
			 * Get the minimum extents of the bounding box.
			 * @return a vector of the minimum extents of the box.
			 */
			Vector3	min() const;
			/**
			 * Get the maximum extents of the bounding box.
			 * @return a vector of the maximum extents of the box.
			 */
			Vector3 max() const;
			/**
			 * Get the 8 vertices that compose the bounding box
			 * @param verts an array of Vector3's to hold the 8 vertices.
			 */
			void vertices(Vector3 * verts) const;
			/**
			 * Set the minimum extents of the bounding box.
			 * @param v the vector containing the minimum extents.
			 */
			void min(const Vector3 & v);
			/**
			 * Set the maximum extents of the bounding box.
			 * @param v the vector containing the maximum extents.
			 */
			void max(const Vector3 & v);
			/**
			 * Set the minimum and maximum extents of the bounding box.
			 * @param min the minimum extents of the bounding box.
			 * @param max the maximum extents of the bounding box.
			 */
			void extents(const Vector3 & min, const Vector3 & max);
			/**
			 * Get the origin of the bounding box.
			 * @return the origin of the bounding box.
			 */
			Vector3	origin() const;

			/**
			 * Extend bounds to include a point.
			 * If the point is already within the bounding box then there is no
			 * change to the bounding box. The point and bounding box are 
			 * assumed to exist in the same coordinate space.
			 * @param point the point to include in the bounding box.
			 */
			void extend(const Vector3 & point);

			// set min & max to bound polygon
//			void		bound(const Polygon & poly);

		private:
			Vector3		min_;
			Vector3		max_;
	};
	
};
