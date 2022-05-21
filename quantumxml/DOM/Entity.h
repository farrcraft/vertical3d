#ifndef INCLUDED_DOM_ENTITY
#define INCLUDED_DOM_ENTITY

#include "Node.h"

namespace DOM
{

	class Entity : virtual public Node
	{
		public:
			Entity();

			DOMString publicId(void) const;
			DOMString systemId(void) const;
			DOMString notationName(void) const;

		protected:
			void publicId(const DOMString & pubid);
			void systemId(const DOMString & sysid);
			void notationName(const DOMString & name);

		private:
			DOMString _publicId;
			DOMString _systemId;
			DOMString _notationName;
	};

}; // end namespace DOM

#endif // INCLUDED_DOM_ENTITY
