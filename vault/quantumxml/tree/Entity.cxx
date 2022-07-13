#include "Entity.h"

using namespace XML;

Entity::Entity() : _type(Unknown)
{
}

void Entity::name(const std::wstring & str)
{
	_name = str;
}

void Entity::value(const std::wstring & val)
{
	_value = val;
}

void Entity::notation(const std::wstring & str)
{
	_notation = str;
}

void Entity::externalID(const ExternalID & extid)
{
	_extId = extid;
}

void Entity::reference(const Reference & ref)
{
	_valrefs.push_back(ref);
}

void Entity::type(EntityType t)
{
	_type = t;
}

std::wstring Entity::name(void) const
{
	return _name;
}

std::wstring Entity::value(void) const
{
	return _value;
}

std::wstring Entity::notation(void) const
{
	return _notation;
}

ExternalID Entity::externalID(void) const
{
	return _extId;
}

Entity::EntityType Entity::type(void) const
{
	return _type;
}
