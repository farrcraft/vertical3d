#ifndef INCLUDED_TALYN_RIBREADER
#define INCLUDED_TALYN_RIBREADER

#include "RenderContext.h"

#include <string>
#include <fstream>

namespace Talyn
{

	class RIBReader
	{
		public:
			RIBReader(const boost::shared_ptr<RenderContext> & rc);

			bool read(const std::string & filename);


		protected:
			void parseToken(const std::string & token);
			std::string readToken();

		private:
			boost::shared_ptr<RenderContext> rc_;
			std::ifstream file_;
	};

}; // end namespace Talyn

#endif // INCLUDED_TALYN_RIBREADER
