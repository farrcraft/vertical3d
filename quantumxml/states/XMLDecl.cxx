#include "XMLDecl.h"

using namespace XML;
using namespace FSM;
using namespace FSMState;

XMLDecl::XMLDecl(FSMParser * fsm) : State<FSMParser>(fsm)
{
	_fsm->addState("XMLDecl", this);
}

// [23]	XMLDecl			::= '<?xml' VersionInfo EncodingDecl? SDDecl? S? '?>'
bool XMLDecl::run(void)
{
	if (!_fsm->stream()->compare(L"<?xml", 5))
	{
		_fsm->stream()->skipChars(5);
	}
	else
	{
		// xml decl is optional so this isn't actually an error condition
		// but we do need to set a flag that there is no xmldecl
		_fsm->document()->setFlag(Document::NoXMLDecl);
		return false;
	}

	if (!version())
		return false;
	if (!encoding())
		return false;
	if (!standalone())
		return false;

	// parse the end of the XMLDecl
	_fsm->stream()->push();
	_fsm->stream()->skipBlanks();
	if (_fsm->stream()->current() == '?' && _fsm->stream()->peek() == '>')
	{
		_fsm->stream()->skipChars(2);
	}
	else
	{
		_fsm->stream()->pop();
		Exception e(Exception::UnknownScanError);
		e.message("Expected end of XMLDecl.");
		_fsm->error(e);
		return false;
	}

	return true;
}

// [24]	VersionInfo		::= S 'version' Eq (' VersionNum ' | " VersionNum ")
bool XMLDecl::version(void)
{
	bool ws = true;
	_fsm->stream()->push();
	// S
	if (!_fsm->stream()->isBlank())
		ws = false;
	else
		_fsm->stream()->skipBlanks();
	// 'version'
	if (_fsm->stream()->compare(L"version", 7))
	{
		Exception e(Exception::UnknownScanError);
		e.message("xmldecl must contain version.");
		_fsm->error(e);
		_fsm->stream()->pop();
		return false;
	}
	if (!ws)
	{
		Exception e(Exception::NoWhitespace);
		_fsm->error(e);
	}
	_fsm->stream()->skipChars(7);
	// Eq
	_fsm->stream()->skipBlanks();
	if (_fsm->stream()->current() != '=')
	{
		Exception e(Exception::NoEq);
		_fsm->error(e);
		return false;
	}
	_fsm->stream()->nextChar();
	_fsm->stream()->skipBlanks();
	int c1 = _fsm->stream()->isSingleDoubleQuote();
	if (c1 == -1)
	{
		Exception e(Exception::NoLiteral);
		_fsm->error(e);
		return false;
	}
	_fsm->stream()->nextChar();
	//  (' VersionNum ' | " VersionNum ")
	std::wstring vers = _fsm->stream()->versionNum();
	int c2 = _fsm->stream()->isSingleDoubleQuote();
	if (c2 == -1)
	{
		Exception e(Exception::NoLiteral);
		_fsm->error(e);
	}
	else if (c1 != c2)
	{
		Exception e(Exception::MismatchedLiteral);
		_fsm->error(e);
	}
	if (c2 != -1)
		_fsm->stream()->nextChar();

	_fsm->document()->version(vers);

	return true;
}

/*
EncodingDecl   ::= S 'encoding' Eq ('"' EncName '"' | "'" EncName "'" )
*/
bool XMLDecl::encoding(void)
{
	bool ws = true;
	_fsm->stream()->push();
	// S
	if (!_fsm->stream()->isBlank())
		ws = false;
	else
		_fsm->stream()->skipBlanks();
	// 'encoding'
	if (_fsm->stream()->compare(L"encoding", 8))
	{
		_fsm->document()->setFlag(Document::NoEncodingDecl);
		_fsm->stream()->pop();
		return true;
	}
	if (!ws)
	{
		Exception e(Exception::NoWhitespace);
		_fsm->error(e);
	}
	_fsm->stream()->skipChars(8);
	// Eq
	_fsm->stream()->skipBlanks();
	if (_fsm->stream()->current() != '=')
	{
		Exception e(Exception::NoEq);
		_fsm->error(e);
		return false;
	}
	_fsm->stream()->nextChar();
	_fsm->stream()->skipBlanks();
	// ('"' EncName '"' | "'" EncName "'" )
	int c1 = _fsm->stream()->isSingleDoubleQuote();
	if (c1 == -1)
	{
		Exception e(Exception::NoLiteral);
		_fsm->error(e);
		return false;
	}
	_fsm->stream()->nextChar();

	std::wstring enc = _fsm->stream()->encName();
	int c2 = _fsm->stream()->isSingleDoubleQuote();
	if (c2 == -1)
	{
		Exception e(Exception::NoLiteral);
		_fsm->error(e);
	}
	else if (c1 != c2)
	{
		// should never happen
		Exception e(Exception::MismatchedLiteral);
		_fsm->error(e);
	}
	if (c2 != -1)
		_fsm->stream()->nextChar();

	_fsm->document()->encoding(enc);

	return true;
}

/*
SDDecl         ::= S 'standalone' Eq (("'" ('yes' | 'no') "'")
                 | ('"' ('yes' | 'no') '"'))
*/
bool XMLDecl::standalone(void)
{
	bool ws = true;
	_fsm->stream()->push();
	// S
	if (!_fsm->stream()->isBlank())
		ws = false;
	else
		_fsm->stream()->skipBlanks();
	// 'standalone'
	if (_fsm->stream()->compare(L"standalone", 10))
	{
		_fsm->document()->setFlag(Document::NoSDDecl);
		_fsm->stream()->pop();
		return true;
	}
	if (!ws)
	{
		Exception e(Exception::NoWhitespace);
		_fsm->error(e);
	}
	_fsm->stream()->skipChars(10);
	// Eq
	_fsm->stream()->skipBlanks();
	if (_fsm->stream()->current() != '=')
	{
		Exception e(Exception::NoEq);
		_fsm->error(e);
		return false;
	}
	_fsm->stream()->nextChar();
	_fsm->stream()->skipBlanks();
	// (("'" ('yes | 'no') "'") | ('"' ('yes' | 'no') '"'))
	int c1 = _fsm->stream()->isSingleDoubleQuote();
	if (c1 == -1)
	{
		Exception e(Exception::NoLiteral);
		_fsm->error(e);
		return false;
	}
	std::wstring str = _fsm->stream()->literal();
	if ((str != L"yes") && (str != L"no"))
	{
		Exception e(Exception::LiteralValue);
		e.message("expected 'yes' or 'no' in standalone decl.");
		_fsm->error(e);
	}

	_fsm->document()->standalone(str);

	return true;
}
