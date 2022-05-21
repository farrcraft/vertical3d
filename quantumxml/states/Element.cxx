#include "Element.h"

#include "../tree/CharData.h"

using namespace XML;
using namespace FSM;

FSMState::Element::Element(FSMParser * fsm) : State<FSMParser>(fsm)
{
	fsm->addState("element", this);
}

/*
[39]   	element		 	    ::=		EmptyElemTag | STag content ETag 	
[40]   	STag			    ::=   	'<' Name (S  Attribute)* S? '>'
[42]   	ETag				::=   	'</' Name  S? '>'
[44]   	EmptyElemTag	    ::=   	'<' Name (S  Attribute)* S? '/>'
*/
bool FSMState::Element::run()
{
	boost::shared_ptr<XML::Element> parent = _fsm->currentElement();

	// STag | EmptyElemTag - '<' Name
	if (_fsm->stream()->current() == '<' && _fsm->stream()->peek() != '/')
	{
		_fsm->stream()->nextChar();
		std::wstring qname = _fsm->stream()->qName();

		boost::shared_ptr<XML::Element> elem(new XML::Element(qname));

		if (!parent)
			_fsm->document()->setRootElement(elem);
		else
		{
			parent->appendChild(elem);
		}

		_fsm->currentElement(elem);
	}
	else
	{
		std::wcout << "expected stag but found: " << _fsm->stream()->current() << " - " << _fsm->stream()->peek() << std::endl;
		return false;
	}

	// (S Attribute)*
	bool has_attribs = true;
	while (has_attribs)
	{
		bool ws = _fsm->stream()->isBlank();
		_fsm->stream()->skipBlanks();
		if (_fsm->callState("Attribute"))
		{
			if (!ws)
			{
				Exception e(Exception::NoWhitespace);
				e.message("expected whitespace before attribute");
				_fsm->error(e);
			}
		}
		else
			has_attribs = false;
	};

	// expect S? ('>' | '/>')
	_fsm->stream()->skipBlanks();

	// end of EmptyElemTag
	if (_fsm->stream()->current() == '/' && _fsm->stream()->peek() == '>')
	{
		_fsm->stream()->skipChars(2);
		_fsm->currentElement(parent);
		return true;
	}
	// end of STag
	if (_fsm->stream()->current() == '>')
	{
		_fsm->stream()->nextChar();

		if (!content())
			return false;
	}

	// [42] ETag			::= '</' Name S? '>'
	if (_fsm->stream()->current() == '<' && _fsm->stream()->peek() == '/')
	{
		_fsm->stream()->skipChars(2);
		std::wstring qname = _fsm->stream()->qName();

		_fsm->stream()->skipBlanks();
		if (_fsm->stream()->current() != '>')
		{
			Exception e(Exception::ExpectedChar);
			e.message("expected '>' in end tag.");
			_fsm->error(e);
		}
		_fsm->stream()->nextChar();
		_fsm->currentElement(parent);
		return true;
	}
	return false;
}

// [43] content		::= CharData? ((element | Reference | CDSect | PI | Comment) CharData?)*
bool FSMState::Element::content()
{
	bool child_content = true;
	while (child_content)
	{
		CharData();

		boost::shared_ptr<Reference> ref;

		if (!_fsm->callState("Comment") && 
			!_fsm->callState("PI") && 
			!_fsm->callState("CDSect") &&
			!_fsm->callState("element") && 
			!_fsm->EntityRef(ref) && 
			!_fsm->CharRef(ref))
			child_content = false;

		if (ref)
			_fsm->currentElement()->appendChild(ref);
	}

	return true;
}

// [14] CharData		::= [^<&]* - ([^<&]* ']]>' [^<&]*) 
bool FSMState::Element::CharData()
{
	unsigned long pos = _fsm->stream()->mark();
	while (_fsm->stream()->current() != '<' 
		&& _fsm->stream()->current() != '&'	
		&& _fsm->stream()->compare(L"]]>", 3))
		_fsm->stream()->nextChar();
	std::wstring str = _fsm->stream()->substr();
	if (pos == _fsm->stream()->position())
		return false;

	boost::shared_ptr<XML::CharData> cdata(new XML::CharData);
	cdata->setData(str);

	boost::shared_ptr<XML::Element> parent = _fsm->currentElement();
	parent->appendChild(cdata);

	// call event handler
//	_eventHandlers->characters(token.getValue());

	return true;
}

