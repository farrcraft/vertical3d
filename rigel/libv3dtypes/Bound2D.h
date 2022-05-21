/*
	Copyright (C) 2001-2004 by Josh Farr
	merkaba_at_quantumfish_dot_com

	This file is part of Vertical|3D.

	Vertical|3D is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Vertical|3D is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Vertical|3D; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef INCLUDED_V3D_BOUND2D_H
#define INCLUDED_V3D_BOUND2D_H

#include "Vector2.h"

namespace v3D
{

	// a 2 dimensional clipped bounding plane
	class Bound2D
	{
		public:
			Bound2D();
			Bound2D(real_t x, real_t y, real_t width, real_t height);
			Bound2D(const Vector2 & position, const Vector2 & size);
			~Bound2D();

			real_t	width(void) const;
			real_t	height(void) const;
			real_t	left(void) const;
			real_t	right(void) const;
			real_t	top(void) const;
			real_t	bottom(void) const;
			Vector2	size(void) const;
			Vector2	position(void) const;

			void	width(real_t width);
			void	height(real_t height);
			void	left(real_t left);
			void	right(real_t right);
			void	top(real_t top);
			void	bottom(real_t bottom);
			void	size(const Vector2 & size);
			void	position(const Vector2 & pos);

			void	shrink(real_t size);

			Bound2D & operator += (const Bound2D & bound);

		private:
			Vector2 _size;
			Vector2	_position;
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_BOUND2D_H
