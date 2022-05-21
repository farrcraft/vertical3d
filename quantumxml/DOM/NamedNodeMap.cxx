#include "NamedNodeMap.h"

#include "Node.h"

using namespace DOM;

unsigned long NamedNodeMap::length(void) const
{
	unsigned long len = static_cast<unsigned long>(_items.size());
	return len;
}

boost::shared_ptr<Node> NamedNodeMap::getNamedItem(const DOMString & name) const
{
	std::map<DOMString, boost::shared_ptr<Node> >::const_iterator iter = _items.find(name);
	if (iter == _items.end())
		return boost::shared_ptr<Node>();
	return iter->second;
}

boost::shared_ptr<Node> NamedNodeMap::setNamedItem(boost::shared_ptr<Node> arg)
{
	boost::shared_ptr<Node> old = getNamedItem(arg->nodeName());

	_items[arg->nodeName()] = arg;

	if (!old)
		return arg;

	return old;
}

boost::shared_ptr<Node> NamedNodeMap::item(unsigned long index) const
{
	if (index < 0 || index >= _items.size())
		return boost::shared_ptr<Node>();

	std::map<DOMString, boost::shared_ptr<Node> >::const_iterator iter = _items.begin();

	for (unsigned long i = 0; i < index; i++)
	{
		iter++;
	}
	return iter->second;
}

boost::shared_ptr<Node> NamedNodeMap::removeNamedItem(const DOMString & name)
{
	std::map<DOMString, boost::shared_ptr<Node> >::iterator iter = _items.find(name);
	if (iter == _items.end())
		return boost::shared_ptr<Node>();

	boost::shared_ptr<Node> old = iter->second;
	_items.erase(iter);
	return old;
}

/*
boost::shared_ptr<Node> NamedNodeMap::getNamedItemNS(const DOMString & namespaceURI, const DOMString & localName) const
{
}

boost::shared_ptr<Node> NamedNodeMap::setNamedItemNS(boost::shared_ptr<Node> arg)
{
}

boost::shared_ptr<Node> NamedNodeMap::removeNamedItemNS(const DOMString & namespaceURI, const DOMString & localName)
{
}
*/
