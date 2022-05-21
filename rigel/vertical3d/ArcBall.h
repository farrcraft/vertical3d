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
#ifndef INCLUDED_V3D_ARCBALL_H
#define INCLUDED_V3D_ARCBALL_H

#include <libv3dtypes/Vector2.h>
#include <libv3dtypes/Vector3.h>
#include <libv3dtypes/Quaternion.h>

namespace v3D
{

	// arcball camera rotation utility
	// based on NeHe lesson 48
	class ArcBall
	{
		public:
			ArcBall();
			~ArcBall();
	
			void click(const Vector2 & point);
			Quaternion drag(const Vector2 & point);
			Vector3 map(const Vector2 & new_point);
			void bounds(float width, float height);

		private:	
			Vector3		_start;
			Vector3		_end;
			float		_width;
			float		_height;
	};


}; // end namespace v3D

#endif // INCLUDED_V3D_ARCBALL_H
