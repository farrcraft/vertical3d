#ifndef INCLUDED_MOYA_VERTEX
#define INCLUDED_MOYA_VERTEX

#ifdef WIN32
#include <3dtypes/Color3.h>
#include <3dtypes/Vector3.h>
#else
#include <vertical3d/3dtypes/Color3.h>
#include <vertical3d/3dtypes/Vector3.h>
#endif

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

				Vector3 point(void) const;
				void point(const Vector3 & v);

			private:
				Vector3			_point;
				Color3			_color;
				Vector3			_normal;
				unsigned int	_bits;
		};

	}; // end namespace Moya

}; // end namespace v3D

#endif // INCLUDED_MOYA_VERTEX
