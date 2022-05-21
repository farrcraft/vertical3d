#include "DoctypeDecl.h"

using namespace XML;
using namespace FSM;
using namespace FSMState;

DoctypeDecl::DoctypeDecl(FSMParser * fsm) : MultiState<FSMParser, DoctypeDecl>(fsm)
{
	MultiState::functor_type dtdAction(this, &DoctypeDecl::doctypedecl);
//	MultiState::functor_type externalIDAction(this, &DoctypeDecl::externalID);
	MultiState::functor_type markupAction(this, &DoctypeDecl::markupdecl);
	MultiState::functor_type sepAction(this, &DoctypeDecl::DeclSep);

	addState("doctypedecl", dtdAction);
//	addState("externalID", externalIDAction);
	addState("markupdecl", markupAction);
	addState("DeclSep", sepAction);
}

// [28] doctypedecl		::= '<!DOCTYPE' S Name (S ExternalID)? S? ('[' (markupdecl | DeclSep)* ']' S?)? '>'
bool DoctypeDecl::doctypedecl(void)
{
	// if we haven't started parsing the DTD yet, look for "'<!DOCTYPE' S Name "
	if (!_fsm->stream()->compare(L"<!DOCTYPE", 9))
	{
		_fsm->stream()->skipChars(9);
		if (!_fsm->stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace in doctype declaration.");
			_fsm->error(e);
			return false;
		}
		else
			_fsm->stream()->skipBlanks();

		std::wstring qname = _fsm->stream()->qName();
		_fsm->document()->setDoctype(qname);
	}
	else
		return false;

	bool ws = true;
 	if (!_fsm->stream()->isBlank())
		ws = false;
	else
		_fsm->stream()->skipBlanks();
	ExternalID extid;
	if (_fsm->getExternalID(extid))
	{
		if (!ws)
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace in external id.");
			_fsm->error(e);
			return false;
		}
		Doctype * dtd = dynamic_cast<Doctype*>(_fsm->document()->doctype());
		dtd->externalID(extid);
	}

	// ('[' (markupdecl | DeclSep)* ']' S?)? '>'
	_fsm->stream()->skipBlanks();
	if (_fsm->stream()->current() == '[')
	{
		_fsm->stream()->nextChar();
		while (_fsm->callState("markupdecl") || _fsm->callState("DeclSep"))
		{
		};
		_fsm->stream()->skipBlanks();
		if (_fsm->stream()->current() == ']')
			_fsm->stream()->nextChar();
		else
		{
			Exception e(Exception::IllegalChar);
			e.message("expected ] after markupdecl");
			_fsm->error(e);
			return false;
		}
	}

	if (_fsm->stream()->current() == '>')
	{
		_fsm->stream()->nextChar();
	}
	else
	{
		Exception e(Exception::IllegalChar);
		e.message("expected > in doctype declaration");
		_fsm->error(e);
		return false;
	}
	return true;
}

// [29]	markupdecl		::= elementdecl | AttlistDecl | EntityDecl | NotationDecl | PI | Comment
bool DoctypeDecl::markupdecl(void)
{
	if (_fsm->callState("elementdecl"))
		return true;
	if (_fsm->callState("AttlistDecl"))
		return true;
	if (_fsm->callState("EntityDecl"))
		return true;
	if (_fsm->callState("NotationDecl"))
		return true;
	if (_fsm->callState("PI"))
		return true;
	if (_fsm->callState("Comment"))
		return true;

	return false;
}

// [28a] DeclSep		::= PEReference | S 
bool DoctypeDecl::DeclSep(void)
{
	if (_fsm->stream()->isBlank())
	{
		_fsm->stream()->skipBlanks();
		return true;
	}

	boost::shared_ptr<Reference> ref;
	if (_fsm->PEReference(ref))
	{
		// do something with newly allocated reference
		return true;
	}

	return false;
}

