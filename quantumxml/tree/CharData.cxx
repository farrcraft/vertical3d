#include "CharData.h"

using namespace XML;

void CharData::setData(const std::wstring & str)
{
	nodeValue(str);
}

bool CharData::sync(std::wofstream & file)
{
	file << nodeValue() << std::endl;
	if (nextSibling() != 0)
	{
		//Node * sibling = dynamic_cast<Node*>(nextSibling());
		dynamic_cast<Node*>(nextSibling().get())->sync(file);
	}
	return true;
}
