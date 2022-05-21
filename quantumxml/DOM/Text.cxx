#include "Text.h"

using namespace DOM;

Text::Text()
{
	nodeType(Node::TEXT_NODE);
	nodeName(L"#text");
}

/*
Text * Text::splitText(unsigned long offset)
{
}
*/
