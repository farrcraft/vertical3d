#include "Element.h"
#include "Attr.h"
#include "Document.h"

#include <iostream>

using namespace DOM;

Element::Element() : Node()
{
	nodeType(Node::ELEMENT_NODE);
}

DOMString Element::tagName(void) const
{
	return nodeName();
}

DOMString Element::getAttribute(const DOMString & name) const
{
	NamedNodeMap * attribs = attributes();
	boost::shared_ptr<Node> attr(attribs->getNamedItem(name));
	if (!attr)
		return L"";
	return attr->nodeValue();
}

boost::shared_ptr<Attr> Element::getAttributeNode(const DOMString & name) const
{
	NamedNodeMap * attribs = attributes();
	boost::shared_ptr<Attr> attr(boost::dynamic_pointer_cast<Attr, Node>(attribs->getNamedItem(name)));
	return attr;
}

void Element::setAttribute(const DOMString & name, const DOMString & value)
{
	boost::shared_ptr<Attr> attr = getAttributeNode(name);
	if (attr)
	{
		attr->nodeValue(value);
		return;
	}
 	attr = ownerDocument()->createAttribute(name);

	boost::weak_ptr<Node> node = self();
	boost::shared_ptr<Element> elem = boost::dynamic_pointer_cast<Element, Node>(node.lock());
	attr->ownerElement(elem);

	attr->value(value);
	NamedNodeMap * attribs = attributes();
	attribs->setNamedItem(attr);
}

boost::shared_ptr<Attr> Element::setAttributeNode(boost::shared_ptr<Attr> newAttr)
{
	boost::weak_ptr<Element> self(boost::enable_shared_from_this<Element>::shared_from_this());
	newAttr->ownerElement(self);
	NamedNodeMap * attribs = attributes();
	boost::shared_ptr<Attr> oldAttr(boost::dynamic_pointer_cast<Attr, Node>(attribs->setNamedItem(newAttr)));

	return oldAttr;
}

void Element::removeAttribute(const DOMString & name)
{
	NamedNodeMap * attribs = attributes();
	attribs->removeNamedItem(name);
}

boost::shared_ptr<Attr> Element::removeAttributeNode(boost::shared_ptr<Attr> oldAttr)
{
	NamedNodeMap * attribs = attributes();
	boost::shared_ptr<Attr> attr(boost::dynamic_pointer_cast<Attr, Node>(attribs->removeNamedItem(oldAttr->nodeName())));
	return attr;
}

/*
NodeList * Element::getElementsByTagName(const DOMString & name)
{
}

DOMString Element::getAttributeNS(const DOMString & namespaceURI, const DOMString & localName)
{
}

void Element::setAttributeNS(const DOMString & namespaceURI, const DOMString & qualifiedName, const DOMString & value)
{
}

void Element::removeAttributeNS(const DOMString & namespaceURI, const DOMString & localName)
{
}

boost::shared_ptr<Attr> Element::getAttributeNodeNS(const DOMString & namespaceURI, const DOMString & localName) const
{
}

boost::shared_ptr<Attr> Element::setAttributeNodeNS(boost::shared_ptr<Attr> newAttr)
{
}

NodeList Element::getElementsByTagNameNS(const DOMString & namespaceURI, const DOMString & localName) const
{
}

bool Element::hasAttributeNS(const DOMString & namespaceURI, const DOMString & localName) const
{
}
*/

bool Element::hasAttribute(const DOMString & name) const
{
	NamedNodeMap * attribs = attributes();
	boost::shared_ptr<Node> attr(attribs->getNamedItem(name));
	if (attr)
		return true;
	return false;
}

// nodename cotains tagname
void Element::tagName(const DOMString & tag)
{
	nodeName(tag);
}

