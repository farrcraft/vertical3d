#include "Document.h"
#include "Attr.h"
#include "CDATASection.h"
#include "Comment.h"
#include "DocumentFragment.h"
#include "ProcessingInstruction.h"
#include "Text.h"
#include "Element.h"
#include "DocumentType.h"

using namespace DOM;

Document::Document() : _doctype(0), _implementation(0)
{
	nodeType(Node::DOCUMENT_NODE);
	nodeName(L"#document");
}

Document::~Document()
{
	if (_doctype != 0)
		delete _doctype;
}

DocumentType * Document::doctype(void) const
{
	return _doctype;
}

DOMImplementation * Document::implementation(void) const
{
	return _implementation;
}

boost::shared_ptr<Element> Document::documentElement(void) const
{
	return _documentElement;
}

boost::shared_ptr<Attr> Document::createAttribute(const DOMString & name)
{
	boost::shared_ptr<Attr> attr(new Attr());
	attr->_weak = attr;
	attr->name(name);
	boost::shared_ptr<Document> doc = boost::dynamic_pointer_cast<Document, Node>(self().lock());
	attr->ownerDocument(doc);
	return attr;
}


boost::shared_ptr<Element> Document::createElement(const DOMString & tagName)
{
	boost::shared_ptr<Element> elem(new Element());
	elem->_weak = elem;
	elem->tagName(tagName);
	boost::shared_ptr<Document> doc = boost::dynamic_pointer_cast<Document, Node>(self().lock());
	elem->ownerDocument(doc);
	return elem;
}

boost::shared_ptr<CDATASection> Document::createCDATASection(const DOMString & data)
{
	boost::shared_ptr<CDATASection> cdsect(new CDATASection());
	cdsect->_weak = cdsect;
	cdsect->nodeValue(data);
	boost::shared_ptr<Document> doc = boost::dynamic_pointer_cast<Document, Node>(self().lock());
	cdsect->ownerDocument(doc);
	return cdsect;
}

boost::shared_ptr<Comment> Document::createComment(const DOMString & data)
{
	boost::shared_ptr<Comment> comment(new Comment());
	comment->_weak = comment;
	comment->nodeValue(data);
	boost::shared_ptr<Document> doc = boost::dynamic_pointer_cast<Document, Node>(self().lock());
	comment->ownerDocument(doc);
	return comment;
}

boost::shared_ptr<DocumentFragment> Document::createDocumentFragment(void)
{
	boost::shared_ptr<DocumentFragment> fragment(new DocumentFragment);
	fragment->_weak = fragment;
	boost::shared_ptr<Document> doc = boost::dynamic_pointer_cast<Document, Node>(self().lock());
	fragment->ownerDocument(doc);
	return fragment;
}

boost::shared_ptr<ProcessingInstruction> Document::createProcessingInstruction(const DOMString & target, const DOMString & data)
{
	boost::shared_ptr<ProcessingInstruction> pi(new ProcessingInstruction());
	pi->_weak = pi;
	pi->target(target);
	pi->data(data);
	boost::shared_ptr<Document> doc = boost::dynamic_pointer_cast<Document, Node>(self().lock());
	pi->ownerDocument(doc);
	return pi;
}


boost::shared_ptr<Text> Document::createTextNode(const DOMString & data)
{
	boost::shared_ptr<Text> txt(new Text());
	txt->_weak = txt;
	txt->nodeValue(data);
	boost::shared_ptr<Document> doc = boost::dynamic_pointer_cast<Document, Node>(self().lock());
	txt->ownerDocument(doc);
	return txt;
}

/*
EntityReference * Document::createEntityReference(const DOMString & name)
{
}
*/

NodeList Document::getElementsByTagName(const DOMString & tagname)
{
	NodeList elems;
	if (!_documentElement)
		return elems;

	if (_documentElement->nodeValue() == tagname)
		elems.push_back(_documentElement);

	return elems;
}

/*
boost::shared_ptr<Node> Document::importNode(Node * importedNode, bool deep)
{
}

boost::shared_ptr<Element> Document::createElementNS(const DOMString & namespaceURI, const DOMString & qualifiedName)
{
}

boost::shared_ptr<Attr> Document::createAttributeNS(const DOMString & namespaceURI, const DOMString & qualifiedName)
{
}

NodeList Document::getElementsByTagNameNS(const DOMString & namespaceURI, const DOMString & localName)
{
}

boost::shared_ptr<Element> Document::getElementById(const DOMString & elementId)
{
}
*/
DOMString Document::xmlEncoding(void) const
{
	return _xmlEncoding;
}

bool Document::xmlStandalone(void) const
{
	return _xmlStandalone;
}

DOMString Document::xmlVersion(void) const
{
	return _xmlVersion;
}

void Document::xmlEncoding(const DOMString & enc)
{
	_xmlEncoding = enc;
}

void Document::xmlStandalone(bool sa)
{
	_xmlStandalone = sa;
}

void Document::xmlVersion(const DOMString & vers)
{
	_xmlVersion = vers;
}

void Document::documentElement(boost::shared_ptr<Element> elem)
{
	_documentElement = elem;
}

void Document::doctype(DocumentType * dtd)
{
	_doctype = dtd;
}
