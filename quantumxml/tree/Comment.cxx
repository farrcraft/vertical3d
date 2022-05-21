#include "Comment.h"

#include <sstream>

using namespace XML;

void Comment::comment(const std::wstring & comm)
{
	nodeValue(comm);
}

bool Comment::sync(std::wofstream & xml_file)
{
	xml_file << "<!--";
	if (nodeValue().size() > 0)
		xml_file << nodeValue();
	xml_file << "-->" << std::endl;

	return true;
}
