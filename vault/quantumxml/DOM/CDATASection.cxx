#include "CDATASection.h"

using namespace DOM;

CDATASection::CDATASection()
{
	nodeType(Node::CDATA_SECTION_NODE);
	nodeName(L"#cdata-section");
}
