#ifndef INCLUDED_V3D_TOOL
#define INCLUDED_V3D_TOOL

#include <string>

namespace v3D
{

	class Tool
	{
		public:
			virtual ~Tool() { }

			virtual void activate(const std::string & name) = 0;
			virtual void deactivate(const std::string & name) = 0;
	};


}; // end namespace v3D

#endif // INCLUDED_V3D_TOOL
