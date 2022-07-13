#include "DocumentType.h"

using namespace DOM;

DocumentType::DocumentType() : Node()
{
	nodeType(Node::DOCUMENT_TYPE_NODE);
}

DOMString DocumentType::name(void) const
{
	return nodeName();
}
/*
NamedNodeMap DocumentType::entities(void) const
{
}

NamedNodeMap DocumentType::notations(void) const
{
}
*/
DOMString DocumentType::publicId(void) const
{
	return _publicId;
}

DOMString DocumentType::systemId(void) const
{
	return _systemId;
}
/*
DOMString DocumentType::internalSubset(void) const
{
}
*/

void DocumentType::name(const DOMString & str)
{
	nodeName(str);
}

void DocumentType::publicId(const DOMString & pubid)
{
	_publicId = pubid;
}

void DocumentType::systemId(const DOMString & sysid)
{
	_systemId = sysid;
}
