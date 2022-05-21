#include "ProcessingInstruction.h"

using namespace DOM;

ProcessingInstruction::ProcessingInstruction() : Node()
{
	nodeType(Node::PROCESSING_INSTRUCTION_NODE);
}

DOMString ProcessingInstruction::target(void) const
{
	return nodeName();
}

DOMString ProcessingInstruction::data(void) const
{
	return nodeValue();
}

// nodename contains target
void ProcessingInstruction::target(const DOMString & str)
{
	nodeName(str);
}

// nodevalue contains data
void ProcessingInstruction::data(const DOMString & str)
{
	nodeValue(str);
}
