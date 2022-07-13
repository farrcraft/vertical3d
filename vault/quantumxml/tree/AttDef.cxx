#include "AttDef.h"

using namespace XML;

AttDef::AttDef(const std::wstring & name) : _name(name)
{
}

void AttDef::type(AttType t)
{
	_type = t;
}

void AttDef::defaultType(DefaultType d)
{
	_default = d;
}

void AttDef::enumeration(const std::wstring & name)
{
	_enumeration.push_back(name);
}

void AttDef::value(const std::wstring & val)
{
	_value = val;
}

