#include "CharacterData.h"

using namespace DOM;

DOMString CharacterData::data(void) const
{
	return nodeValue();
}
/*
unsigned long CharacterData::length(void) const
{
}

DOMString CharacterData::substringData(unsigned long offset, unsigned long count) const
{
}

void CharacterData::appendData(const DOMString & arg)
{
}

void CharacterData::insertData(unsigned long offset, const DOMString & arg)
{
}

void CharacterData::deleteData(unsigned long offset, unsigned long count)
{
}

void CharacterData::replaceData(unsigned long offset, unsigned long count, const DOMString & arg)
{
}
*/
