#include "Parser.h"
#include "FSMParser.h"

using namespace XML;

Parser::Parser() : _parserFlags(StateUninitialized) 
{
	_pimpl = new FSMParser(this);
}

Parser::~Parser()
{
	delete _pimpl;
}

bool Parser::initialize()
{
	// save the validation & wfc settings
	bool validate = false;
	if (_parserFlags & ValidationEnabled)
		validate = true;

	bool wfc = false;
	if (_parserFlags & WFCEnabled)
		wfc = true;

	// reset the state bits (clears the StateUninitialized bit)
	_parserFlags = StateReady;

	// restore validation & wfc settings
	if (validate)
		_parserFlags |= ValidationEnabled;
	if (wfc)
		_parserFlags |= WFCEnabled;

	return true;
}

void Parser::quiet(bool quiet_output)
{
	if (quiet_output)
		_parserFlags |= QuietOutput;
	else
		_parserFlags &= ~QuietOutput;
}

void Parser::error(const Exception & e)
{
	if (e.code() == Exception::WFC && !isWFCEnabled())
		return;
	if (!(_parserFlags & QuietOutput))
		std::cout << e.error() << ": " << e.message() << std::endl;
	_parserFlags |= FatalError;
}

void Parser::enableWFC(bool wfc_state)
{
	if (wfc_state)
		_parserFlags |= WFCEnabled;
	else
		_parserFlags &= ~WFCEnabled;
}

bool Parser::isWFCEnabled() const
{
	if (_parserFlags & WFCEnabled)
		return true;
	return false;
}

void Parser::enableValidation(bool validation_state)
{
	if (validation_state)
		_parserFlags |= ValidationEnabled;
	else
		_parserFlags &= ~ValidationEnabled;
}

bool Parser::isValidationEnabled() const
{
	if (_parserFlags & ValidationEnabled)
		return true;
	return false;
}

void Parser::uri(const std::string & url)
{
	if (_parserFlags & StateDirty)
		initialize();

	_stream.setURI(url);
}

std::string Parser::uri() const
{
	return _stream.getURI();
}

boost::shared_ptr<Document> Parser::document()
{
	return _pimpl->document();
}

Stream * Parser::stream()
{
	return &_stream;
}

bool Parser::parse()
{
	// if the parser has been used or hasn't been initialized, it needs to be now
	if ((_parserFlags & StateDirty) || (_parserFlags & StateUninitialized))
		if (!initialize())
			return false;

	// make sure there's something in the stream to parse
	if (_stream.end())
	{
		Exception e(Exception::StreamError);
		e.message("nothing in the stream to parse!");
		_pimpl->error(e);
		return false;
	}

	if (!(_parserFlags & StateDirty))
		_parserFlags |= StateDirty;

	if (!(_parserFlags & StateParsing))
		_parserFlags |= StateParsing;

	_pimpl->run();


	// do WFC error checks
	if (_parserFlags & WFCEnabled)
	{
		if (!document()->documentElement())
		{
			Exception e(Exception::WFC);
			e.message("document element missing!");
			_pimpl->error(e);
		}
	}

	if (_parserFlags & FatalError)
	{
		Exception e(Exception::UnknownScanError);
		e.message("fatal parser error!");
		_pimpl->error(e);
		return false;
	}

	return true;
}

// parse an entire document
bool Parser::parse(const std::string & url)
{
	// setup _stream
	if (!_stream.setURI(url))
	{
		Exception e(Exception::StreamError);
		e.message("error setting stream uri!");
		_pimpl->error(e);
		return false;
	}

	return parse();
}

#ifdef DONTIGNOREBLOCK

/*
conditionalSect ::= includeSect | ignoreSect
includeSect     ::= '<![' S? 'INCLUDE' S? '[' extSubsetDecl ']]>'
ignoreSect      ::= '<![' S? 'IGNORE' S? '[' ignoreSectContents* ']]>'
*/
bool Parser::conditionalSect(Token &token)
{
	if (!_stream.compare(L"<![", 3))
	{
		_stream.skipChars(3);
		_stream.skipBlanks();
		bool include = false;
		if (!_stream.compare(L"INCLUDE", 7))
		{
			_stream.skipChars(7);
			include = true;
		}
		else if (!_stream.compare(L"IGNORE", 6))
			_stream.skipChars(6);
		else
		{
			Exception e(Exception::UnknownScanError);
			error(e);
		}
		_stream.skipBlanks();
		if (_stream.current() != '[')
		{
			Exception e(Exception::ExpectedChar);
			e._message = "[";
			error(e);
		}
		else
			_stream.nextChar();
		if (include)
		{
			// scan extSubsetDecl
		}
		else
		{
			// scan ignoreSectContents
		}
		if (!_stream.compare(L"]]>", 3))
		{
			_stream.skipChars(3);
			token.type(CondSectClose);
			return true;
		}
		else
		{
			Exception e(Exception::ExpectedChar);
			e._message = "]]>";
			error(e);
		}
		token.type(CondSectOpen);
		return true;
	}
	return false;
}

/*
ignoreSectContents ::= Ignore ('<![' ignoreSectContents ']]>' Ignore)*
Ignore             ::= Char* - (Char* ('<![' | ']]>') Char*)
*/
bool Parser::ignoreSectContents(Token &token)
{
	return false;
}

/*
SkipLit        ::= ('"' [^"]* '"') | ("'" [^']* "'")
*/
bool Parser::skipLit(Token &token)
{
	wstring lit = _stream.literal();
	if (!lit.empty())
		return true;
	return false;
}

/*
extSubset      ::= TextDecl? extSubsetDecl
extSubsetDecl  ::= ( markupdecl | conditionalSect | PEReference | S )*
*/

/*
TextDecl       ::= '<?xml' VersionInfo? EncodingDecl S? '?>'
*/
bool Parser::textDecl(Token &token)
{
	if (!_stream.compare(L"<?xml", 5))
	{
		_stream.skipChars(5);
		token.type(::XMLDecl);
		return true;
	}
	if (getLastToken().type() == ::XMLDecl)
	{
		if (version(token))
			return true;
	}
	if (getLastToken().type() == ::XMLDecl || getLastToken().type() == Version)
	{
		bool ret = encoding(token);
		if (!ret)
		{
			Exception e(Exception::ExpectedChar);
			e._message = "EncodingDecl";
			error(e);
		}
		_stream.skipBlanks();
		if (_stream.current() == '?' && _stream.peek() == '>')
			_stream.skipChars(2);
		return ret;
	}
	return false;
}
#endif 

bool Parser::write(const std::string & url, boost::shared_ptr<DOM::Document> doc)
{
	URL u(url);
	bool result = false;
	if (doc)
		result = dynamic_cast<XML::Document*>(doc.get())->sync(u);
	else
	{
		if (_pimpl->document())
			result = _pimpl->document()->sync(u);
	}
	return result;
}

bool Parser::write(const std::string & url)
{
	return write(url, _pimpl->document());
}
