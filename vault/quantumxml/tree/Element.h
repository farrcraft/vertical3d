#ifndef INCLUDED_XML_ELEMENT
#define INCLUDED_XML_ELEMENT

#include <vector>

#include "Node.h"
#include "Attribute.h"

#include "../DOM/Element.h"

namespace XML
{

	class Element : public Node, public DOM::Element
	{
		public:
			Element();
			Element(const std::wstring & name);
			~Element();

			bool sync(std::wofstream & file);
	};

}; // end namespace XML

#endif // INCLUDED_XML_ELEMENT
