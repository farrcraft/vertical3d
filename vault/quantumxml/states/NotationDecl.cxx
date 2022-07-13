#include "NotationDecl.h"

#include "../tree/Notation.h"

using namespace XML;
using namespace FSM;
using namespace FSMState;

NotationDecl::NotationDecl(FSMParser * fsm) : State<FSMParser>(fsm)
{
	_fsm->addState("NotationDecl", this);
}


// [75] ExternalID		::= 'SYSTEM' S SystemLiteral | 'PUBLIC' S PubidLiteral S SystemLiteral  
// [83] PublicID		::= 'PUBLIC' S PubidLiteral 
bool NotationDecl::getID(ExternalID & extid)
{
	bool missing_whitespace = false;
	bool pubid = false;

	if (!_fsm->stream()->compare(L"SYSTEM", 6))
	{
		_fsm->stream()->skipChars(6);
		if (!_fsm->stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace in external id.");
			_fsm->error(e);
			return false;
		}
		else
			_fsm->stream()->skipBlanks();
		std::wstring sys = _fsm->stream()->systemLiteral();
		extid.systemId(sys);
		return true;
	}

	if (!_fsm->stream()->compare(L"PUBLIC", 6))
	{
		_fsm->stream()->skipChars(6);
		if (!_fsm->stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace in external id.");
			_fsm->error(e);
			return false;
		}
		else
			_fsm->stream()->skipBlanks();
		std::wstring pub = _fsm->stream()->pubidLiteral();

		// next sequence must be S SystemLiteral
		if (!_fsm->stream()->isBlank())
		{
			missing_whitespace = true;
		}
		else
			_fsm->stream()->skipBlanks();
		std::wstring sys = _fsm->stream()->systemLiteral();
		if (sys.empty())
			pubid = true;

		extid.publicId(pub);
		extid.systemId(sys);
	}

	if (!pubid && missing_whitespace)
	{
		Exception e(Exception::NoWhitespace);
		e.message("expected whitespace in external id.");
		_fsm->error(e);
		return false;
	}

	return true;
}

// [82]   	NotationDecl ::=   	'<!NOTATION' S  Name  S (ExternalID | PublicID) S? '>'
bool NotationDecl::run(void)
{
	if (!_fsm->stream()->compare(L"<!NOTATION", 10))
	{

		_fsm->stream()->skipChars(10);
		if (!_fsm->stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace after <!NOTATION");
			_fsm->error(e);
		}
		else
			_fsm->stream()->skipBlanks();
		std::wstring name = _fsm->stream()->name();
		Notation notation(name);

		if (!_fsm->stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace in NotationDecl");
			_fsm->error(e);
		}
		else
			_fsm->stream()->skipBlanks();

		// get either externalid or publicid
		ExternalID extid;
		if (getID(extid))
		{
			notation.externalID(extid);
		}
		else
		{
			Exception e(Exception::LiteralValue);
			e.message("expected 'PUBLIC' or 'SYSTEM' in NotationDecl");
			_fsm->error(e);
			return false;
		}

		_fsm->stream()->skipBlanks();
		if (_fsm->stream()->current() != '>')
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace in NotationDecl");
			_fsm->error(e);
		}
		else
			_fsm->stream()->nextChar();

		return true;
	}

	return false;
}
