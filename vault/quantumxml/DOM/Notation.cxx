#include "Notation.h"

using namespace DOM;

Notation::Notation() : Node()
{
	nodeType(Node::NOTATION_NODE);
}

DOMString Notation::publicId(void) const
{
	return _publicId;
}

DOMString Notation::systemId(void) const
{
	return _systemId;
}

void Notation::publicId(const DOMString & pubid)
{
	_publicId = pubid;
}

void Notation::systemId(const DOMString & sysid)
{
	_systemId = sysid;
}

