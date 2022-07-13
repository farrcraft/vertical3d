#include "CDSect.h"

using namespace XML;

#include <sstream>

CDSect::CDSect() : CDATASection()
{
}

void CDSect::setData(const std::wstring & str)
{
	nodeValue(str);
}

bool CDSect::sync(std::wofstream & xml_file)
{
	xml_file << "<![CDATA['";
	if (data().size() > 0)
		xml_file << data();
	xml_file << "]]>" << std::endl;

	return true;
}
