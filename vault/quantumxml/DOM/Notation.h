#ifndef INCLUDED_DOM_NOTATION
#define INCLUDED_DOM_NOTATION

#include "Node.h"

namespace DOM
{

	class Notation : virtual public Node
	{
		public:
			Notation();

			DOMString publicId(void) const;
			DOMString systemId(void) const;

		protected:
			void publicId(const DOMString & pubid);
			void systemId(const DOMString & sysid);

		private:
			DOMString _publicId;
			DOMString _systemId;
	};

}; // end namespace DOM

#endif // INCLUDED_DOM_NOTATION
