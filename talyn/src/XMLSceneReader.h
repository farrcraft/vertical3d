#ifndef INCLUDED_TALYN_XMLSCENEREADER
#define INCLUDED_TALYN_XMLSCENEREADER

#include "RenderContext.h"

#include <string>

namespace Talyn
{

	class XMLSceneReader
	{
		public:
			bool read(const boost::shared_ptr<RenderContext> & rc, const std::string & filename);

		protected:
	};

}; // end namespace Talyn

#endif // INCLUDED_TALYN_XMLSCENEREADER
