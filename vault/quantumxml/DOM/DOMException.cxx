#include "DOMException.h"

using namespace DOM;

// ExceptionCode
const unsigned short DOM::INDEX_SIZE_ERR                 = 1;
const unsigned short DOM::DOMSTRING_SIZE_ERR             = 2;
const unsigned short DOM::HIERARCHY_REQUEST_ERR          = 3;
const unsigned short DOM::WRONG_DOCUMENT_ERR             = 4;
const unsigned short DOM::INVALID_CHARACTER_ERR          = 5;
const unsigned short DOM::NO_DATA_ALLOWED_ERR            = 6;
const unsigned short DOM::NO_MODIFICATION_ALLOWED_ERR    = 7;
const unsigned short DOM::NOT_FOUND_ERR                  = 8;
const unsigned short DOM::NOT_SUPPORTED_ERR              = 9;
const unsigned short DOM::INUSE_ATTRIBUTE_ERR            = 10;
// Introduced in DOM Level 2:
const unsigned short DOM::INVALID_STATE_ERR              = 11;
// Introduced in DOM Level 2:
const unsigned short DOM::SYNTAX_ERR                     = 12;
// Introduced in DOM Level 2:
const unsigned short DOM::INVALID_MODIFICATION_ERR       = 13;
// Introduced in DOM Level 2:
const unsigned short DOM::NAMESPACE_ERR                  = 14;
// Introduced in DOM Level 2:
const unsigned short DOM::INVALID_ACCESS_ERR             = 15;


DOMException::DOMException(unsigned short c) : _code(c)
{
}

unsigned short DOMException::code() const
{
	return _code;
}
