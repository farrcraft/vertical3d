#include "ProcessingInstruction.h"

#include <sstream>

using namespace XML;

void ProcessingInstruction::setTarget(const std::wstring & tgt)
{
	target(tgt);
}

void ProcessingInstruction::setData(const std::wstring & dta)
{
	data(dta);
}

bool ProcessingInstruction::sync(std::wofstream & xml_file)
{
	xml_file << "<?" << target();
	if (data().size() > 0)
		xml_file << " " << data();
	xml_file << "?>" << std::endl;

	return true;
}
