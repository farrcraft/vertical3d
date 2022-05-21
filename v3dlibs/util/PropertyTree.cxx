#include "PropertyTree.h"
#include "PropertyTreeReader.h"

PropertyTree EMPTY_PROPERTY_TREE;

PropertyTree::PropertyTree()
{
}

PropertyTree::~PropertyTree()
{
}

bool PropertyTree::load(const std::string & filename)
{
	PropertyTreeReader ptree_reader;
	bool rval = ptree_reader.read(filename, this);

	return rval;
}

void PropertyTree::set(const std::string & key, const std::string & value)
{
	key_ = key;
	value_ = value;
}

PropertyTree & PropertyTree::insert(const std::string & key, const std::string & value)
{
	PropertyTree tree;
	tree.set(key, value);
	children_.push_back(tree);

	return children_.back();
}

bool PropertyTree::empty(void) const
{
	return (key_.empty() && value_.empty() && children_.empty());
}

const PropertyTree & PropertyTree::find(const std::string & key) const
{
	// check if key string is composed of multiple keys separated by a "." character
	std::string::size_type pos = key.find(".");
	if (pos != std::string::npos) // multiple key path
	{
		// get the left-most key (root/parent key)
		std::string str = key.substr(0, pos);
		// and the remaining subkey string on the right hand side
		std::string next_key = key.substr(pos+1);
		// find the root key in our list of children nodes
		const_iterator iter = children_.begin();
		for (; iter != children_.end(); iter++)
		{
			if (iter->key_ == str) // found root key
			{
				// recurse on node with the remaining subkey string
				return iter->find(next_key);
			}
		}
	}
	else // key contains no subkeys
	{
		// find the child node with matching key
		const_iterator iter = children_.begin();
		for (; iter != children_.end(); iter++)
		{
			if (iter->key_ == key)
				return *iter;
		}
	}
	// the key did not match any nodes in this tree
	return EMPTY_PROPERTY_TREE;
}

std::string PropertyTree::get(const std::string & key) const
{
	const PropertyTree & ptree = find(key);
	return ptree.value_;
}

std::string PropertyTree::key(void) const
{
	return key_;
}

std::string PropertyTree::value(void) const
{
	return value_;
}

void PropertyTree::print(const std::string & depth)
{
	std::cout << depth << "Tree pair: " << key_ << " - " << value_ << std::endl;

	iterator iter = children_.begin();
	for (; iter != children_.end(); iter++)
	{
		iter->print(depth + "   ");
	}
}

PropertyTree::const_iterator PropertyTree::begin(void) const
{
	const_iterator iter = children_.begin();
	if (iter == children_.end())
		return children_.begin();

	for (; iter != children_.end(); iter++)
	{
		if (iter->key() != "<xmlattr>")
			break;
	}
	return iter;
}

PropertyTree::const_iterator PropertyTree::end(void) const
{
	return children_.end();
}

PropertyTree::iterator PropertyTree::begin(void)
{
	iterator iter = children_.begin();
	if (iter == children_.end())
		return children_.begin();

	for (; iter != children_.end(); iter++)
	{
		if (iter->key() != "<xmlattr>")
			break;
	}
	return iter;
}

PropertyTree::iterator PropertyTree::end(void)
{
	return children_.end();
}
