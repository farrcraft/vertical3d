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
#ifndef INCLUDED_MOYA_COLOR3_H
#define INCLUDED_MOYA_COLOR3_H

#include "3DMath.h"

namespace v3D
{

	class Color3
	{
		public:
			Color3();
			Color3(real_t r, real_t g, real_t b);
			Color3(const Color3 & c);
			Color3(real_t c[3]);
			Color3(real_t c);
			~Color3();

			real_t 			operator[](unsigned int i) const;

		private:
			real_t	_color[3];
	};

};

#endif // INCLUDED_MOYA_COLOR3_H
