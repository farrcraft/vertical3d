#include "Notation.h"

using namespace XML;

Notation::Notation(const std::wstring & name)
{
	nodeName(name);
}

Notation::~Notation()
{
}

void Notation::externalID(const ExternalID & extid)
{
	publicId(extid.publicId());
	systemId(extid.systemId());
}
