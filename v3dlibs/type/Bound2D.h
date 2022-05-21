/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <glm/glm.hpp>

namespace v3d::type
{

	/**
	 * A 2 dimensional clipped bounding plane object
	 */
	class Bound2D
	{
		public:
			/**
			 * Construct a new 2D bounding object
			 *
			 * @param x the left position of the clipping plane
			 * @param y the top position of the clipping plane
			 * @param width the width of the bound
			 * @param height the height of the bound
			 */
			Bound2D(float x, float y, float width, float height);
			/**
			 * Construct a new 2D bounding object
			 *
			 * @param position the top left corner of the clipping plane
			 * @param size the width and height of the bounded area
			 */
			Bound2D(const glm::vec2 & position, const glm::vec2 & size);

			/**
			 * Get the size of the bounding volume
			 *
			 * @return the width and height of the bounded area
			 */
			glm::vec2 size() const;
			/**
			 * Get the position of the bounding volume
			 *
			 * @return the top left corner of the bounding box
			 */
			glm::vec2 position() const;
			/**
			 * Shrink the bounding box by a given amount.
			 *
			 * @param size the amount to shrink the box by
			 */
			void shrink(float size);
			/**
			 * Expand the bounding box by a given amount.
			 *
			 * @param size the amount to expand the box by
			 */
			void expand(float size);
			/**
			 * Determine if a given point intersects the bounding volume
			 *
			 * @param point the point to check for intersection
			 * @return whether the point is inside the bounding box (true) or not (false)
			 */
			bool intersect(const glm::vec2 & point);
			/**
			 * Add the size of another bounding volume to this one
			 *
			 * @param bound the bounding area to add
			 * @return a reference to the updated bounding object
			 */
			Bound2D & operator += (const Bound2D & bound);

		private:
			glm::vec2 size_;
			glm::vec2	position_;
	};

};
