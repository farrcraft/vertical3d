#include "DOMImplementation.h"
#include "../tree/Document.h"
/*
#include <algorithm>
#include <cctype>
*/
using namespace DOM;

bool DOMImplementation::hasFeature(const DOMString & feature, const DOMString & version)
{
	DOMString f = feature;
	//std::transform(f.begin(), f.end(), f, tolower);

	if (f.compare(L"core") && version.compare(L"2.0"))
		return true;
	else if (f.compare(L"xml") && version.compare(L"2.0"))
		return true;
	return false;
}
/*
DocumentType * DOMImplementation::createDocumentType(const DOMString & qualifiedName, const DOMString & publicId, const DOMString & systemId)
{
}
*/
boost::shared_ptr<Document> DOMImplementation::createDocument(const DOMString & namespaceURI, const DOMString & qualifiedName, DocumentType * doctype)
{
	_document.reset(new XML::Document());
	_document->weak(_document);
	return _document;
}
