#include "DocumentFragment.h"

using namespace DOM;

DocumentFragment::DocumentFragment()
{
	nodeType(Node::DOCUMENT_FRAGMENT_NODE);
	nodeName(L"#document-fragment");
}

