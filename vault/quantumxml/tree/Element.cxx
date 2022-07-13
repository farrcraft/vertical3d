#include "Element.h"

#include <sstream>

using namespace XML;

Element::Element()
{ 
}

Element::Element(const std::wstring & name)
{
	nodeName(name);
}

Element::~Element() 
{
}

bool Element::sync(std::wofstream & file)
{
	file << "<" << nodeName();

	DOM::NamedNodeMap * attribs = attributes();
	unsigned int count = attribs->length();

	for (unsigned int i = 0; i < count; i++)
	{
		boost::shared_ptr<Attribute> att(dynamic_cast<Attribute*>(attribs->item(i).get()));
		file << " ";
		att->sync(file);
	}

	if (firstChild() == 0)
		file << "/>" << std::endl;
	else
	{
		file << ">" << std::endl;
		Node * child = dynamic_cast<Node*>(firstChild().get());
		child->sync(file);
		file << "</" << nodeName() << ">" << std::endl;
	}
	if (nextSibling() != 0)
	{
		Node * sibling = dynamic_cast<Node*>(nextSibling().get());
		sibling->sync(file);
	}
	return true;
}
