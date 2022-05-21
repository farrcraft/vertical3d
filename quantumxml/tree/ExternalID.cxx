#include "ExternalID.h"

#include <sstream>

using namespace XML;

void ExternalID::publicId(const std::wstring & pid)
{
	_publicId = pid;
}

void ExternalID::systemId(const std::wstring & sid)
{
	_systemId = sid;
}

std::wstring ExternalID::publicId(void) const
{
	return _publicId;
}

std::wstring ExternalID::systemId(void) const
{
	return _systemId;
}

bool ExternalID::sync(std::wofstream & xml_file)
{
	if (_systemId.size() > 0)
	{
		if (_publicId.size() == 0)
			xml_file << "SYSTEM " << _systemId;
		else
			xml_file << "PUBLIC " << _publicId << " " << _systemId;
	}
	return true;
}
