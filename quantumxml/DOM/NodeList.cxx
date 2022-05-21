#include "NodeList.h"
#include "DOMException.h"

using namespace DOM;


boost::shared_ptr<Node> NodeList::item(unsigned long index) const
{
	if (index >= _nodes.size())
	{
		DOMException ex(INDEX_SIZE_ERR);
		throw ex;
	}
	return _nodes[index];
}

unsigned long NodeList::length(void) const
{
	return static_cast<unsigned long>(_nodes.size());
}

void NodeList::push_back(boost::shared_ptr<Node> node)
{
	_nodes.push_back(node);
}