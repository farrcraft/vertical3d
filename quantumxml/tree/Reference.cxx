#include "Reference.h"

using namespace XML;

Reference::Reference() : _type(None)
{
}

void Reference::type(RefType t)
{
	_type = t;
}

void Reference::value(const std::wstring & val)
{
	_value = val;
}

Reference::RefType Reference::type(void) const
{
	return _type;
}

std::wstring Reference::value(void) const
{
	return _value;
}

