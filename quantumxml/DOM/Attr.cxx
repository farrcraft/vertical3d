#include "Attr.h"

using namespace DOM;

Attr::Attr() : Node(), _specified(true)
{
	nodeType(Node::ATTRIBUTE_NODE);
}

DOMString Attr::name(void) const
{
	return nodeName();
}

bool Attr::specified(void) const
{
	return _specified;
}

DOMString Attr::value(void)
{
	return nodeValue();
}

boost::shared_ptr<Element> Attr::ownerElement(void) const
{
	return _ownerElement.lock();
}


// nodename contains name
void Attr::name(const DOMString & str)
{
	nodeName(str);
}

// nodevalue contains value
void Attr::value(const DOMString & str)
{
	nodeValue(str);
}

void Attr::specified(bool spec)
{
	_specified = spec;
}

void Attr::ownerElement(boost::weak_ptr<Element> elem)
{
	_ownerElement = elem;
}

