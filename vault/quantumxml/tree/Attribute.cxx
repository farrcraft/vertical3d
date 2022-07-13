#include "Attribute.h"

#include <sstream>

using namespace XML;

Attribute::Attribute() 
{ 
}

Attribute::Attribute(const std::wstring & name)
{
	nodeName(name);
}

Attribute::~Attribute() 
{ 
}

void Attribute::name(const std::wstring & str)
{
	nodeName(str);
}

void Attribute::value(const std::wstring & str)
{
	nodeValue(str);
}

bool Attribute::sync(std::wofstream & file)
{
	file << nodeName() << "=\"" << nodeValue() << "\"";
	return true;
}
