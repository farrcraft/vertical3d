#include "Node.h"

using namespace DOM;

Node::Node()
{
}

Node::~Node()
{
	/*
	if (_firstChild != 0)
	{
		Node * node = _firstChild;
		for (; node != 0; )
		{
			Node * nxt = node->_nextSibling;
			delete node;
			node = nxt;
		}
	}
	*/
}

DOMString Node::nodeName(void) const
{
	return _nodeName;
}

DOMString Node::nodeValue(void) const
{
	return _nodeValue;
}

void Node::nodeValue(const DOMString & val)
{
	_nodeValue = val;
}

unsigned short Node::nodeType(void) const
{
	return _nodeType;
}

boost::weak_ptr<Node> Node::parentNode(void) const
{
	return _parentNode;
}
/*
NodeList * Node::childNodes(void) const
{
	return & _childNodes;
}
*/
boost::shared_ptr<Node> Node::firstChild(void) const
{
	return _firstChild;
}

boost::shared_ptr<Node> Node::lastChild(void) const
{
	return _lastChild;
}

boost::shared_ptr<Node> Node::previousSibling(void) const
{
	return _previousSibling;
}

boost::shared_ptr<Node> Node::nextSibling(void) const
{
	return _nextSibling;
}

NamedNodeMap * Node::attributes(void) const
{
	return const_cast<NamedNodeMap*>(&_attributes);
}

boost::shared_ptr<Document> Node::ownerDocument(void) const
{
	boost::shared_ptr<Document> doc = _ownerDocument.lock();
	return doc;
}

/*
boost::shared_ptr<Node> Node::insertBefore(boost::shared_ptr<Node> newChild, boost::shared_ptr<Node> refChild)
{
}

boost::shared_ptr<Node> Node::replaceChild(boost::shared_ptr<Node> newChild, boost::shared_ptr<Node> oldChild)
{
}

boost::shared_ptr<Node> Node::removeChild(boost::shared_ptr<Node> oldChild)
{
}
*/

void Node::nodeName(const DOMString & str)
{
	_nodeName = str;
}

void Node::nodeType(unsigned short type)
{
	_nodeType = type;
}

void Node::ownerDocument(boost::weak_ptr<Document> owner)
{
	_ownerDocument = owner;
}

void Node::appendSibling(boost::shared_ptr<Node> node)
{
	node->_parentNode = _parentNode;

	if (!_nextSibling)
	{
		_nextSibling = node;
		node->_previousSibling.reset(this);
	}
	else
	{
		boost::shared_ptr<Node> last = _nextSibling;
		while (last != 0)
		{
			if (last->_nextSibling != 0)
				last = last->_nextSibling;
			else
			{
				last->_nextSibling = node;
				node->_previousSibling = last;
				return;
			}
		}
	}
}

boost::weak_ptr<Node> Node::self(void) const
{
	return _weak;
}

void Node::weak(boost::shared_ptr<Node> _this)
{
	_weak = _this;
}

boost::shared_ptr<Node> Node::appendChild(boost::shared_ptr<Node> newChild)
{
	if (_lastChild)
	{
		newChild->_previousSibling = _lastChild;
		_lastChild->_nextSibling = newChild;
	}
	else
	{
		_firstChild = newChild;
	}
	newChild->_parentNode = _weak; //.reset(this);
	_lastChild = newChild;

	return newChild;
}

bool Node::hasChildNodes(void) const
{
	if (_firstChild)
		return true;
	return false;
}
/*
boost::shared_ptr<Node> Node::cloneNode(bool deep)
{
}

void Node::normalize(void)
{
}

bool Node::isSupported(const DOMString & feature, const DOMString & version)
{
}
*/
DOMString Node::namespaceURI(void) const
{
	return _namespaceURI;
}

DOMString Node::prefix(void) const
{
	return _prefix;
}

void Node::prefix(const DOMString & p)
{
	_prefix = p;
}
/*
DOMString Node::localName(void) const
{
	return _localName;
}
*/
bool Node::hasAttributes(void) const
{
	if (_attributes.length() > 0)
		return true;
	return false;
}

