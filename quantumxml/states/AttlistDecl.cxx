#include "AttlistDecl.h"

using namespace XML;
using namespace FSM;
using namespace FSMState;

AttlistDecl::AttlistDecl(FSMParser * fsm) : State<FSMParser>(fsm)
{
	_fsm->addState("AttlistDecl", this);
}

// [52] AttlistDecl		::= '<!ATTLIST' S Name AttDef* S? '>'
bool AttlistDecl::run(void)
{
	if (!_fsm->stream()->compare(L"<!ATTLIST", 9))
	{
		_fsm->stream()->skipChars(9);
		if (!_fsm->stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace after <!ATTLIST");
			_fsm->error(e);
		}
		else
			_fsm->stream()->skipBlanks();

		// element name
		std::wstring qname = _fsm->stream()->qName();
		AttList attribs(qname);
		while (AttDef(attribs))
		{
		}
		_fsm->stream()->skipBlanks();

		if (_fsm->stream()->current() != '>')
		{
			Exception e(Exception::ExpectedChar);
			e.message("expected > in AttlistDecl");
			_fsm->error(e);
		}
		else
			_fsm->stream()->nextChar();
		return true;
	}
	return false;
}

// [53] AttDef         ::= S (QName | NSAttName) S AttType S DefaultDecl
bool AttlistDecl::AttDef(AttList & attribs)
{
	if (!_fsm->stream()->isBlank())
	{
		Exception e(Exception::NoWhitespace);
		e.message("expected whitespace in AttDef");
		_fsm->error(e);
	}
	else
		_fsm->stream()->skipBlanks();

	std::wstring name = _fsm->stream()->nsAttName();
	if (name.empty())
		name = _fsm->stream()->qName();

	XML::AttDef def(name);

	if (!_fsm->stream()->isBlank())
	{
		Exception e(Exception::NoWhitespace);
		e.message("expected whitespace before AttType");
		_fsm->error(e);
	}
	else
		_fsm->stream()->skipBlanks();

	if (!AttType(def))
	{
	}

	if (!_fsm->stream()->isBlank())
	{
		Exception e(Exception::NoWhitespace);
		e.message("expected whitespace before AttType");
		_fsm->error(e);
	}
	else
		_fsm->stream()->skipBlanks();

	if (!DefaultDecl(def))
	{
	}

	if (_fsm->stream()->current() == '>')
	{
		_fsm->stream()->nextChar();
		return true;
	}
	return false;
}

// [60] DefaultDecl     ::= '#REQUIRED' | '#IMPLIED' | (('#FIXED' S)? AttValue)
bool AttlistDecl::DefaultDecl(XML::AttDef & def)
{
	if (!_fsm->stream()->compare(L"#REQUIRED", 9))
	{
		_fsm->stream()->skipChars(9);
		def.defaultType(AttDef::Required);
		return true;
	}
	if (!_fsm->stream()->compare(L"#IMPLIED", 8))
	{
		_fsm->stream()->skipChars(9);
		def.defaultType(AttDef::Implied);
		return true;
	}
	if (!_fsm->stream()->compare(L"#FIXED", 6))
	{
		_fsm->stream()->skipChars(6);
		def.defaultType(AttDef::Fixed);
		if (!_fsm->stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace in DefaultDecl");
			_fsm->error(e);
		}
		else
			_fsm->stream()->skipBlanks();
		// get AttValue
		return true;
	}
	else
	{
		def.defaultType(AttDef::Value);
		// get AttValue
		return true;
	}
	return false;
}

/*
AttType        ::= StringType | TokenizedType | EnumeratedType
StringType     ::= 'CDATA'
TokenizedType  ::= 'ID' | 'IDREF' | 'IDREFS'
	             | 'ENTITY' | 'ENTITIES' | 'NMTOKEN' | 'NMTOKENS'
EnumeratedType ::= NotationType | Enumeration
NotationType   ::= 'NOTATION' S '(' S? Name (S? '|' S? Name)* S? ')'
Enumeration    ::= '(' S? Nmtoken (S? '|' S? Nmtoken)* S? ')'
*/
bool AttlistDecl::AttType(XML::AttDef & def)
{
	if (!_fsm->stream()->compare(L"CDATA", 5))
	{
		_fsm->stream()->skipChars(5);
		def.type(AttDef::CData);
		return true;
	}
	if (!_fsm->stream()->compare(L"IDREFS", 6))
	{
		_fsm->stream()->skipChars(6);
		def.type(AttDef::IdRefs);
		return true;
	}
	if (!_fsm->stream()->compare(L"IDREF", 5))
	{
		_fsm->stream()->skipChars(5);
		def.type(AttDef::IdRef);
		return true;
	}
	if (!_fsm->stream()->compare(L"ID", 2))
	{
		_fsm->stream()->skipChars(2);
		def.type(AttDef::Id);
		return true;
	}
	if (!_fsm->stream()->compare(L"ENTITY", 6))
	{
		_fsm->stream()->skipChars(6);
		def.type(AttDef::Entity);
		return true;
	}
	if (!_fsm->stream()->compare(L"ENTITIES", 8))
	{
		_fsm->stream()->skipChars(8);
		def.type(AttDef::Entities);
		return true;
	}
	if (!_fsm->stream()->compare(L"NMTOKENS", 8))
	{
		_fsm->stream()->skipChars(8);
		def.type(AttDef::Nmtokens);
		return true;
	}
	if (!_fsm->stream()->compare(L"NMTOKEN", 7))
	{
		_fsm->stream()->skipChars(7);
		def.type(AttDef::Nmtoken);
		return true;
	}
	if (!_fsm->stream()->compare(L"NOTATION", 8))
	{
		// NotationType   ::= 'NOTATION' S '(' S? Name (S? '|' S? Name)* S? ')'
		_fsm->stream()->skipChars(8);
		def.type(AttDef::Notation);
		if (!_fsm->stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace after NOTATION");
			_fsm->error(e);
		}
		else
			_fsm->stream()->skipBlanks();
		if (_fsm->stream()->current() != '(')
		{
			Exception e(Exception::ExpectedChar);
			e.message("(");
			_fsm->error(e);
		}
		else
			_fsm->stream()->nextChar();
		_fsm->stream()->skipBlanks();
		std::wstring name = _fsm->stream()->name();
		def.enumeration(name);

		while (true)
		{
			_fsm->stream()->skipBlanks();
			if (_fsm->stream()->current() == ')')
			{
				_fsm->stream()->nextChar();
				return true;
			}
			if (_fsm->stream()->current() == '|')
				_fsm->stream()->nextChar();
			else
			{
				Exception e(Exception::ExpectedChar);
				e.message("|");
				_fsm->error(e);
				return false;
			}
			_fsm->stream()->skipBlanks();
			name = _fsm->stream()->name();
			def.enumeration(name);
		}

		return true;
	}
	if (_fsm->stream()->current() == '(')
	{
		// Enumeration    ::= '(' S? Nmtoken (S? '|' S? Nmtoken)* S? ')'
		_fsm->stream()->nextChar();
		def.type(AttDef::Enumerated);
		_fsm->stream()->skipBlanks();
		std::wstring name = _fsm->stream()->nmtoken();
		def.enumeration(name);

		while (true)
		{
			_fsm->stream()->skipBlanks();
			if (_fsm->stream()->current() == ')')
			{
				_fsm->stream()->nextChar();
				return true;
			}
			if (_fsm->stream()->current() == '|')
				_fsm->stream()->nextChar();
			else
			{
				Exception e(Exception::ExpectedChar);
				e.message("|");
				_fsm->error(e);
				return false;
			}
			_fsm->stream()->skipBlanks();
			name = _fsm->stream()->nmtoken();
			def.enumeration(name);
		}

		return true;
	}

	return false;
}
