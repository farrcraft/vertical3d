#ifndef INCLUDED_DOM_DOMIMPLEMENTATION
#define INCLUDED_DOM_DOMIMPLEMENTATION

#include "DOMString.h"

#include <boost/shared_ptr.hpp>

namespace DOM
{

	class Document;
	class DocumentType;

	class DOMImplementation
	{
		public:
			bool hasFeature(const DOMString & feature, const DOMString & version);
			// Introduced in DOM Level 2:
			DocumentType * createDocumentType(const DOMString & qualifiedName, const DOMString & publicId, const DOMString & systemId);
			// Introduced in DOM Level 2:
			boost::shared_ptr<Document> createDocument(const DOMString & namespaceURI, const DOMString & qualifiedName, DocumentType * doctype);

		private:
			boost::shared_ptr<Document> _document;
	};

}; // end namespace DOM

#endif // INCLUDED_DOM_DOMIMPLEMENTATION
