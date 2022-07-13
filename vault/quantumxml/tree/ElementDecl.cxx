#include "ElementDecl.h"

using namespace XML;

void ElemDecl::elementName(const std::wstring & elem)
{
	_elementName = elem;
}

void ElemDecl::contentspec(ContentSpec spec)
{
	_contentspec = spec;
}

