#ifndef INCLUDED_DOM_ELEMENT
#define INCLUDED_DOM_ELEMENT

#include "Node.h"
#include <boost/enable_shared_from_this.hpp>

namespace DOM
{

	class Attr;

	class Element : virtual public Node, public boost::enable_shared_from_this<Element>
	{
		friend class Document;

		public:
			typedef boost::shared_ptr<Element> ptr;

			Element();

			DOMString tagName(void) const;
			DOMString getAttribute(const DOMString & name)const;
			void setAttribute(const DOMString & name, const DOMString & value);
			void removeAttribute(const DOMString & name);
			boost::shared_ptr<Attr> getAttributeNode(const DOMString & name) const;
			boost::shared_ptr<Attr> setAttributeNode(boost::shared_ptr<Attr> newAttr);
			boost::shared_ptr<Attr> removeAttributeNode(boost::shared_ptr<Attr> oldAttr);
			NodeList * getElementsByTagName(const DOMString & name);
			// Introduced in DOM Level 2:
			DOMString getAttributeNS(const DOMString & namespaceURI, const DOMString & localName);
			// Introduced in DOM Level 2:
			void setAttributeNS(const DOMString & namespaceURI, const DOMString & qualifiedName, const DOMString & value);
			// Introduced in DOM Level 2:
			void removeAttributeNS(const DOMString & namespaceURI, const DOMString & localName);
			// Introduced in DOM Level 2:
			boost::shared_ptr<Attr> getAttributeNodeNS(const DOMString & namespaceURI, const DOMString & localName) const;
			// Introduced in DOM Level 2:
			boost::shared_ptr<Attr> setAttributeNodeNS(boost::shared_ptr<Attr> newAttr);
			// Introduced in DOM Level 2:
			NodeList * getElementsByTagNameNS(const DOMString & namespaceURI, const DOMString & localName) const;
			// Introduced in DOM Level 2:
			bool hasAttribute(const DOMString & name) const;
			// Introduced in DOM Level 2:
			bool hasAttributeNS(const DOMString & namespaceURI, const DOMString & localName) const;

		protected:
			// nodename cotains tagname
			void tagName(const DOMString & tag);
	};

}; // end namespace DOM

#endif // INCLUDED_DOM_ELEMENT
