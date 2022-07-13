#include "Node.h"

#include <sstream>

using namespace XML;

Node::Node()
{ 
}

Node::~Node() 
{
}

void Node::addSibling(boost::shared_ptr<Node> node)
{
	appendSibling(node);
}

bool Node::sync(std::wofstream & file)
{
	return true;
}
