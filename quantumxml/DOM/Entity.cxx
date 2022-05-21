#include "Entity.h"

using namespace DOM;

Entity::Entity() : Node()
{
	nodeType(Node::ENTITY_NODE);
}

DOMString Entity::publicId(void) const
{
	return _publicId;
}

DOMString Entity::systemId(void) const
{
	return _systemId;
}

DOMString Entity::notationName(void) const
{
	return _notationName;
}

void Entity::publicId(const DOMString & pubid)
{
	_publicId = pubid;
}

void Entity::systemId(const DOMString & sysid)
{
	_systemId = sysid;
}

void Entity::notationName(const DOMString & name)
{
	_notationName = name;
}
