#ifndef INCLUDED_DOM_NAMEDNODEMAP
#define INCLUDED_DOM_NAMEDNODEMAP

#include "DOMString.h"

#include <map>
#include <boost/shared_ptr.hpp>

namespace DOM
{

	class Node;

	class NamedNodeMap
	{
		public:
			boost::shared_ptr<Node> getNamedItem(const DOMString & name) const;
			boost::shared_ptr<Node> setNamedItem(boost::shared_ptr<Node> arg);
			boost::shared_ptr<Node> removeNamedItem(const DOMString & name);
			boost::shared_ptr<Node> item(unsigned long index) const;
			unsigned long length(void) const;
			// Introduced in DOM Level 2:
			boost::shared_ptr<Node> getNamedItemNS(const DOMString & namespaceURI, const DOMString & localName) const;
			// Introduced in DOM Level 2:
			boost::shared_ptr<Node> setNamedItemNS(boost::shared_ptr<Node> arg);
			// Introduced in DOM Level 2:
			boost::shared_ptr<Node> removeNamedItemNS(const DOMString & namespaceURI, const DOMString & localName);

		private:
			std::map<DOMString, boost::shared_ptr<Node> > _items;
	};

}; // end namespace DOM

#endif // INCLUDED_DOM_NAMEDNODEMAP
