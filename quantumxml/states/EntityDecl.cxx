#include "EntityDecl.h"

using namespace XML;
using namespace FSM;
using namespace FSMState;

EntityDecl::EntityDecl(FSMParser * fsm) : State<FSMParser>(fsm)
{
	_fsm->addState("EntityDecl", this);
}

// [70] EntityDecl		::= GEDecl | PEDecl
bool EntityDecl::run(void)
{
	Entity entity;
	if (GEDecl(entity) || PEDecl(entity))
		return true;

	return false;
}

// [71] GEDecl			::= '<!ENTITY' S Name S EntityDef S? '>'
bool EntityDecl::GEDecl(Entity & entity)
{
	if (!_fsm->stream()->compare(L"<!ENTITY", 8))
	{
		_fsm->stream()->skipChars(8);
		if (!_fsm->stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace after <!ENTITY");
			_fsm->error(e);
		}
		else
			_fsm->stream()->skipBlanks();

		std::wstring name = _fsm->stream()->name();
		entity.name(name);

		if (!_fsm->stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace after EntityDecl Name");
			_fsm->error(e);
		}
		else
			_fsm->stream()->skipBlanks();

		if (!EntityDef(entity))
		{
			return false;
		}

		_fsm->stream()->skipBlanks();

		if (_fsm->stream()->current() != '>')
		{
			Exception e(Exception::ExpectedChar);
			e.message("expected > in EntityDecl");
			_fsm->error(e);
		}
		else
			_fsm->stream()->nextChar();

		return true;
	}
	return false;
}

// [72] PEDecl			::= '<!ENTITY' S '%' S Name S PEDef S? '>'
bool EntityDecl::PEDecl(Entity & entity)
{
	if (!_fsm->stream()->compare(L"<!ENTITY", 8))
	{
		_fsm->stream()->skipChars(8);
		if (!_fsm->stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace after <!ENTITY");
			_fsm->error(e);
		}
		else
			_fsm->stream()->skipBlanks();

		if (_fsm->stream()->current() == '%')
		{
			_fsm->stream()->nextChar();
			if (!_fsm->stream()->isBlank())
			{
				Exception e(Exception::NoWhitespace);
				e.message("expected whitespace after % in PEDecl");
				_fsm->error(e);
			}
			else
				_fsm->stream()->skipBlanks();
		}

		std::wstring name = _fsm->stream()->name();
		entity.name(name);

		if (!_fsm->stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace after EntityDecl Name");
			_fsm->error(e);
		}
		else
			_fsm->stream()->skipBlanks();

		if (!PEDef(entity))
		{
			return false;
		}

		_fsm->stream()->skipBlanks();

		if (_fsm->stream()->current() != '>')
		{
			Exception e(Exception::ExpectedChar);
			e.message("expected > in EntityDecl");
			_fsm->error(e);
		}
		else
			_fsm->stream()->nextChar();

		return true;
	}
	return false;
}

// [74] PEDef			::= EntityValue | ExternalID
bool EntityDecl::PEDef(Entity & entity)
{
	ExternalID extid;
	if (_fsm->getExternalID(extid))
	{
		entity.externalID(extid);
		return true;
	}
	if (EntityValue(entity))
		return true;

	return false;
}

// [73] EntityDef		::= EntityValue | (ExternalID NDataDecl?)
bool EntityDecl::EntityDef(Entity & entity)
{
	ExternalID extid;
	if (_fsm->getExternalID(extid))
	{
		// [76] NDataDecl		::= S 'NDATA' S Name 
		bool ws = false;
		_fsm->stream()->push();
		if (_fsm->stream()->isBlank())
		{
			_fsm->stream()->skipBlanks();
			ws = true;
		}
		if (!_fsm->stream()->compare(L"NDATA", 5))
		{
			_fsm->stream()->skipChars(5);
			if (!ws)
			{
				Exception e(Exception::NoWhitespace);
				e.message("expected whitespace before NDATA");
				_fsm->error(e);
			}
			if (!_fsm->stream()->isBlank())
			{
				Exception e(Exception::NoWhitespace);
				e.message("expected whitespace after NDATA");
				_fsm->error(e);
			}
			else
				_fsm->stream()->skipBlanks();
			std::wstring name = _fsm->stream()->name();
			entity.notation(name);
		}
		else
			_fsm->stream()->pop();

		return true;
	}

	if (EntityValue(entity))
		return true;
	return false;
}

/*
[9]  EntityValue	::= '"' ([^%&"] | PEReference | Reference)* '"'  
					 |  "'" ([^%&'] | PEReference | Reference)* "'" 
*/
bool EntityDecl::EntityValue(Entity & entity)
{
	int c1 = _fsm->stream()->isSingleDoubleQuote();
	if (c1 == -1)
	{
		Exception e(Exception::ExpectedChar);
		e.message("expected ' or \" in EntityValue");
		_fsm->error(e);
		return false;
	}
	_fsm->stream()->setDelimiter(_fsm->stream()->current());
	_fsm->stream()->nextChar();

	while (_fsm->stream()->current() != _fsm->stream()->getDelimiter())
	{
		boost::shared_ptr<Reference> ref;
		if (_fsm->PEReference(ref) || _fsm->EntityRef(ref) || _fsm->CharRef(ref))
		{
		}
		else
		{
			unsigned long start = _fsm->stream()->mark();
			while (_fsm->stream()->current() != '%' && _fsm->stream()->current() != '&' 
					&& _fsm->stream()->current() != _fsm->stream()->getDelimiter())
				_fsm->stream()->nextChar();
			if (start != _fsm->stream()->position())
			{
				std::wstring value = _fsm->stream()->substr();
				ref.reset(new Reference);
				ref->type(Reference::None);
				ref->value(value);
			}
		}
	}

	if (_fsm->stream()->current() == _fsm->stream()->getDelimiter())
	{
		_fsm->stream()->nextChar();
	}
	else
	{
		Exception e(Exception::ExpectedChar);
		e.message("expected ' or \" in EntityValue");
		_fsm->error(e);
		return false;
	}
	return true;
}
