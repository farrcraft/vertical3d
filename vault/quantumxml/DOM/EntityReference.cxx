#include "EntityReference.h"

using namespace DOM;

EntityReference::EntityReference() : Node()
{
	nodeType(Node::ENTITY_REFERENCE_NODE);
}
