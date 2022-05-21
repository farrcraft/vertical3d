#include "PropertyTreeReader.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

void PropertyTreeReader::read_attributes(PropertyTree & tree, DOM::Node::ptr node)
{
	if (node->hasAttributes())
	{
		PropertyTree & attrib_tree = tree.insert("<xmlattr>", "");

		DOM::NamedNodeMap * attribs = node->attributes();
		unsigned int count = attribs->length();
		for (unsigned int i = 0; i < count; i++)
		{
			DOM::Node::ptr att = attribs->item(i);
			if (!att)
				std::cout << "NULL Attr Cast!" << std::endl;
			attrib_tree.insert(DOM::wide2str(att->nodeName()), DOM::wide2str(att->nodeValue()));
		}
	}

}

bool PropertyTreeReader::read(const std::string & filename, PropertyTree * tree)
{
	XML::Parser parser;
	// resolve the full path from the filename
	boost::filesystem::path full_path = boost::filesystem::system_complete(filename);
	// convert path to the url format that the xml parser expects
	std::string url = "file://" + full_path.string();

	try 
	{
		if (!parser.parse(url))
		{
			std::cout << "Error loading property tree" << std::endl;
			return false;
		}
	}
	catch (...) 
	{
		std::cout << "Unexpected Exception" << std::endl;
		return false;
	}

	DOM::Element::ptr root = parser.document()->documentElement();
	if (!root)
	{
		std::cout << "poperty tree reader - doc elem null!" << std::endl;
		return false;
	}

	std::string key = DOM::wide2str(root->tagName());
	PropertyTree & child = tree->insert(key, "");
	read_attributes(child, root);

	DOM::Node::ptr sections = root->firstChild();
	if (sections)
		read(child, sections);

	return true;
}

void PropertyTreeReader::read(PropertyTree & parent, DOM::Node::ptr sections)
{
	for (; sections != 0; sections = sections->nextSibling())
	{
		if (sections->nodeType() == DOM::Node::ELEMENT_NODE)
		{
			std::string key = DOM::wide2str(sections->nodeName());
			std::string value = DOM::wide2str(sections->nodeValue());
			PropertyTree & child = parent.insert(key, value);

			read_attributes(child, sections);

			if (sections->hasChildNodes())
				read(child, sections->firstChild());
		}
	}
}
