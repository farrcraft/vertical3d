#include "AttList.h"

using namespace XML;

AttList::AttList(const std::wstring & elem) : _name(elem)
{
}

std::wstring AttList::name(void) const
{
	return _name;
}

void AttList::attdef(const AttDef & def)
{
	_attdefs.push_back(def);
}
