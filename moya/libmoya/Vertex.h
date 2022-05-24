/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../../v3dlibs/type/Color3.h"
#include "../../v3dlibs/type/Vector3.h"

namespace v3D
{
	namespace Moya
	{
		class Vertex
		{
			public:
				Vertex();
				~Vertex();

				enum Bits
				{
					HAS_COLOR = (1 << 1),
					HAS_NORMAL = (1 << 2),
					HAS_TEX_COORD = (1 << 3)
				};

				v3d::type::Vector3 point(void) const;
				void point(const v3d::type::Vector3 & v);

			private:
				v3d::type::Vector3 _point;
				v3d::type::Color3 _color;
				v3d::type::Vector3 _normal;
				unsigned int	_bits;
		};

	}; // end namespace Moya

}; // end namespace v3D

