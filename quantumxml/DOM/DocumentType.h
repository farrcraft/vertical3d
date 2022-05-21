#ifndef INCLUDED_DOM_DOCUMENTTYPE
#define INCLUDED_DOM_DOCUMENTTYPE

#include "Node.h"

namespace DOM
{

	class DocumentType : virtual public Node
	{
		public:
			DocumentType();

			DOMString name(void) const;
			NamedNodeMap entities(void) const;
			NamedNodeMap notations(void) const;
			// Introduced in DOM Level 2:
			DOMString publicId(void) const;
			// Introduced in DOM Level 2:
			DOMString systemId(void) const;
			// Introduced in DOM Level 2:
			DOMString internalSubset(void) const;

		protected:
			void name(const DOMString & str);
			void publicId(const DOMString & pubid);
			void systemId(const DOMString & sysid);

		private:
			DOMString _publicId;
			DOMString _systemId;
	};

}; // end namespace DOM

#endif // INCLUDED_DOM_DOCUMENTTYPE
