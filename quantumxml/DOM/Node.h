#ifndef INCLUDED_DOM_NODE
#define INCLUDED_DOM_NODE

#include "DOMString.h"
#include "NodeList.h"
#include "NamedNodeMap.h"

#include <boost/weak_ptr.hpp>

namespace DOM
{

	class Document;

	class Node
	{
		public:
			typedef boost::shared_ptr<Node> ptr;

			enum NodeType
			{
				ELEMENT_NODE = 1,
				ATTRIBUTE_NODE,
				TEXT_NODE,
				CDATA_SECTION_NODE,
				ENTITY_REFERENCE_NODE,
				ENTITY_NODE,
				PROCESSING_INSTRUCTION_NODE,
				COMMENT_NODE,
				DOCUMENT_NODE,
				DOCUMENT_TYPE_NODE,
				DOCUMENT_FRAGMENT_NODE,
				NOTATION_NODE
			};

			Node();
			virtual ~Node();

			DOMString nodeName(void) const;
			DOMString nodeValue(void) const;
			void nodeValue(const DOMString & val);
			unsigned short nodeType(void) const;
			boost::weak_ptr<Node> parentNode(void) const;
			NodeList * childNodes(void) const;
			boost::shared_ptr<Node> firstChild(void) const;
			boost::shared_ptr<Node> lastChild(void) const;
			boost::shared_ptr<Node> previousSibling(void) const;
			boost::shared_ptr<Node> nextSibling(void) const;
			NamedNodeMap * attributes(void) const;
			// Modified in DOM Level 2:
			boost::shared_ptr<Document> ownerDocument(void) const;

			boost::shared_ptr<Node> insertBefore(boost::shared_ptr<Node> newChild, boost::shared_ptr<Node> refChild);
			boost::shared_ptr<Node> replaceChild(boost::shared_ptr<Node> newChild, boost::shared_ptr<Node> oldChild);
			boost::shared_ptr<Node> removeChild(boost::shared_ptr<Node> oldChild);
			boost::shared_ptr<Node> appendChild(boost::shared_ptr<Node> newChild);
			bool hasChildNodes(void) const;
			boost::shared_ptr<Node> cloneNode(bool deep);
			// Modified in DOM Level 2:
			void normalize(void);
			// Introduced in DOM Level 2:
			bool isSupported(const DOMString & feature, const DOMString & version);
			// Introduced in DOM Level 2:
			DOMString namespaceURI(void) const;
			// Introduced in DOM Level 2:
			DOMString prefix(void) const;
			void prefix(const DOMString & p);
			// Introduced in DOM Level 2:
			DOMString localName(void) const;
			// Introduced in DOM Level 2:
			bool hasAttributes(void) const;

			boost::weak_ptr<Node> self(void) const;
			void weak(boost::shared_ptr<Node> _this);

		protected:
			void nodeName(const DOMString & str);
			void nodeType(unsigned short type);
			void ownerDocument(boost::weak_ptr<Document> owner);
			void appendSibling(boost::shared_ptr<Node> node);

			boost::weak_ptr<Node> _weak;

		private:
			DOMString _nodeName;
			DOMString _nodeValue;
			unsigned short _nodeType;
			boost::weak_ptr<Node> _parentNode;
			NodeList _childNodes;
			boost::shared_ptr<Node>	_firstChild;
			boost::shared_ptr<Node> _lastChild;
			boost::shared_ptr<Node> _previousSibling;
			boost::shared_ptr<Node> _nextSibling;
			NamedNodeMap _attributes;
			DOMString _namespaceURI;
			DOMString _prefix;
			boost::weak_ptr<Document> _ownerDocument;
	};

}; // end namespace DOM

#endif // INCLUDED_DOM_NODE
