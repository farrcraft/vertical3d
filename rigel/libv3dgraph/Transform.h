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
#ifndef INCLUDED_V3D_DAG_TRANSFORM_H
#define INCLUDED_V3D_DAG_TRANSFORM_H

#include <libv3dtypes/Vector3.h>
#include <libv3dtypes/Matrix4.h>
#include <libv3dtypes/Quaternion.h>

namespace v3D
{

	namespace DAG
	{

		class Transform
		{
			public:
				Transform();
				virtual ~Transform();

				void		scale(const Vector3 & s);
				void		rotation(const Quaternion & r);
				virtual void translation(const Vector3 & t);

				Vector3 	scale(void) const;
				Quaternion	rotation(void) const;
				virtual Vector3 translation(void) const;

				Matrix4		matrix(void) const;

			private:
				Vector3		_translation;
				Vector3		_scale;
				Quaternion	_rotation;
		};

	};

}; // end namespace v3D

#endif // INCLUDED_V3D_DAG_TRANSFORM_H
