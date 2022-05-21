#include "Comment.h"

using namespace DOM;

Comment::Comment()
{
	nodeType(Node::COMMENT_NODE);
	nodeName(L"#comment");
}
