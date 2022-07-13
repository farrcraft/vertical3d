#include "Attribute.h"

using namespace XML;
using namespace FSM;

FSMState::Attribute::Attribute(FSMParser * fsm) : State<FSMParser>(fsm)
{
	fsm->addState("Attribute", this);
}

// [41] Attribute		::= Name Eq AttValue
// [25] Eq				::= S? '=' S? 
bool FSMState::Attribute::run(void)
{
	boost::shared_ptr<XML::Element> elem = _fsm->currentElement();

	std::wstring name = _fsm->stream()->nsAttName();
	if (name.empty())
		name = _fsm->stream()->qName();
	if (name.empty())
		return false;

	boost::shared_ptr<XML::Attribute> attr(new XML::Attribute(name));

	// Eq is next
	_fsm->stream()->skipBlanks();
	if (_fsm->stream()->current() != '=')
	{
		Exception e(Exception::NoEq);
		e.message("expected = after Attribute Name");
		_fsm->error(e);
	}
	else
		_fsm->stream()->nextChar();
	_fsm->stream()->skipBlanks();

	if (!AttValue(attr))
		return false;

	elem->setAttributeNode(attr);

	return true;
}

// [10] AttValue		::= '"' ([^<&"] | Reference)* '"'  |  "'" ([^<&'] | Reference)* "'" 
bool FSMState::Attribute::AttValue(boost::shared_ptr<XML::Attribute> attr)
{
	// handle the delimiter character - '"' | "'"
	int c1 = _fsm->stream()->isSingleDoubleQuote();
	if (c1 == -1)
	{
		Exception e(Exception::NoLiteral);
		e.message("expected quote in AttValue");
		_fsm->error(e);
		return false;
	}

	_fsm->stream()->setDelimiter(_fsm->stream()->current());
	_fsm->stream()->nextChar();

	unsigned long start = _fsm->stream()->mark();
	while (_fsm->stream()->current() != _fsm->stream()->getDelimiter())
	{
		boost::shared_ptr<Reference> ref;
		//if (!_fsm->callState("Reference"))
		if (!_fsm->EntityRef(ref) && !_fsm->CharRef(ref))
		{
			if (_fsm->stream()->current() == '<')
			{
				Exception e(Exception::IllegalChar);
				e.message("illegal '<' in attvalue.");
				_fsm->error(e);
			}
			_fsm->stream()->nextChar();
		}
		else
		{
			// deal with the inline reference
		}
	}
	if (start != _fsm->stream()->position())
	{
		std::wstring value = _fsm->stream()->substr();
		attr->value(value);
	}
	if (_fsm->stream()->current() == _fsm->stream()->getDelimiter())
	{
		_fsm->stream()->nextChar();
		return true;
	}
	return false;
}
