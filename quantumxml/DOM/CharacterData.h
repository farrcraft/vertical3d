#ifndef INCLUDED_DOM_CHARACTERDATA
#define INCLUDED_DOM_CHARACTERDATA

#include "Node.h"

namespace DOM
{
	class CharacterData : virtual public Node
	{
		public:
			DOMString data(void) const;
			unsigned long length(void) const;
			DOMString substringData(unsigned long offset, unsigned long count) const;
			void appendData(const DOMString & arg);
			void insertData(unsigned long offset, const DOMString & arg);
			void deleteData(unsigned long offset, unsigned long count);
			void replaceData(unsigned long offset, unsigned long count, const DOMString & arg);
	};

};

#endif // INCLUDED_DOM_CHARACTERDATA
