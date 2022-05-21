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
#ifndef INCLUDED_CONSTRUCTIONPLANE_H
#define INCLUDED_CONSTRUCTIONPLANE_H

#include <libv3dcore/Camera.h>

namespace v3D
{

	class ConstructionPlane
	{
		public:
			ConstructionPlane();
			~ConstructionPlane();

			void	size(float s);
			void	spacing(float s);
			void	intervals(int m);
			void	infinite(bool i);
			void	autoscale(bool a);

			float	size(void);
			float	spacing(void);
			int		intervals(void);
			bool	infinite(void);
			bool	autoscale(void);

			void	draw(Camera * cam);

		private:
			float	_size;
			float	_spacing;
			int		_majorIntervals;
			bool	_infinite;
			bool	_autoscale;
			int		_scaleFactor;
	};

}; // end namespace v3D

#endif // INCLUDED_CONSTRUCTIONPLANE_H
