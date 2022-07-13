#include "Exception.h"

using namespace XML;

const std::string ExceptionMessages[] = 
{	
	"unkown scan error", 
	"no whitespace", 
	"expected '='",
	"expected literal '\"' or \"'\"",
	"expected literal value",
	"mismatched literal",
	"expected character",
	"illegal character",
	"wfc error"
};

Exception::Exception(ErrorCode c) : code_(c) 
{ 
}

std::string Exception::error() const
{
	return ExceptionMessages[code_];
}

std::string Exception::message() const
{
	return message_;
}

void Exception::message(const std::string & msg)
{
	message_ = msg;
}

unsigned short Exception::code() const
{
	return code_;
}
