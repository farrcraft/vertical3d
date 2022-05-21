#ifndef INCLUDED_DOM_DOCUMENT
#define INCLUDED_DOM_DOCUMENT

#include "Node.h"

namespace DOM
{

	class DocumentType;
	class DOMImplementation;
	class Text;
	class Comment;
	class CDATASection;
	class ProcessingInstruction;
	class Attr;
	class EntityReference;
	class Element;
	class DocumentFragment;

	class Document : virtual public Node
	{
		public:
			Document();
			virtual ~Document();

			DocumentType * doctype(void) const;
			DOMImplementation * implementation(void) const;
			boost::shared_ptr<Element> documentElement(void) const;

			boost::shared_ptr<Element> createElement(const DOMString & tagName);
			boost::shared_ptr<DocumentFragment> createDocumentFragment(void);
			boost::shared_ptr<Text> createTextNode(const DOMString & data);
			boost::shared_ptr<Comment> createComment(const DOMString & data);
			boost::shared_ptr<CDATASection> createCDATASection(const DOMString & data);
			boost::shared_ptr<ProcessingInstruction> createProcessingInstruction(const DOMString & target, const DOMString & data);
			boost::shared_ptr<Attr> createAttribute(const DOMString & name);
			boost::shared_ptr<EntityReference> createEntityReference(const DOMString & name);
			NodeList getElementsByTagName(const DOMString & tagname);
			// Introduced in DOM Level 2:
			boost::shared_ptr<Node> importNode(Node * importedNode, bool deep);
			// Introduced in DOM Level 2:
			boost::shared_ptr<Element> createElementNS(const DOMString & namespaceURI, const DOMString & qualifiedName);
			// Introduced in DOM Level 2:
			boost::shared_ptr<Attr> createAttributeNS(const DOMString & namespaceURI, const DOMString & qualifiedName);
			// Introduced in DOM Level 2:
			NodeList getElementsByTagNameNS(const DOMString & namespaceURI, const DOMString & localName);
			// Introduced in DOM Level 2:
			boost::shared_ptr<Element> getElementById(const DOMString & elementId);

			// Introduced in DOM Level 3:
			DOMString xmlEncoding(void) const;
			// Introduced in DOM Level 3:
			bool xmlStandalone(void) const;
			// Introduced in DOM Level 3:
			DOMString xmlVersion(void) const;

		protected:
			void xmlEncoding(const DOMString & enc);
			void xmlStandalone(bool sa);
			void xmlVersion(const DOMString & vers);
			void doctype(DocumentType * dtd);
			void documentElement(boost::shared_ptr<Element> elem);

		private:
			DocumentType * _doctype;
			DOMImplementation * _implementation;
			boost::shared_ptr<Element> _documentElement;
			DOMString _xmlEncoding;
			bool _xmlStandalone;
			DOMString _xmlVersion;
	};

}; // end namespace DOM

#endif // INCLUDED_DOM_DOCUMENT
