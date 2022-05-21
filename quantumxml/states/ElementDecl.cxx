#include "ElementDecl.h"

using namespace XML;
using namespace FSM;
using namespace FSMState;

ElementDecl::ElementDecl(FSMParser * fsm) : MultiState<FSMParser, ElementDecl>(fsm), _elem(0)
{
	MultiState::functor_type elemAction(this, &ElementDecl::elementdecl);
	MultiState::functor_type contentAction(this, &ElementDecl::contentspec);
	MultiState::functor_type mixedAction(this, &ElementDecl::Mixed);
	MultiState::functor_type childrenAction(this, &ElementDecl::children);

	addState("elementdecl", elemAction);
	addState("contentspec", contentAction);
	addState("Mixed", mixedAction);
	addState("children", childrenAction);
}

// [45]	elementdecl		::= '<!ELEMENT' S QName S contentspec S? '>'
bool ElementDecl::elementdecl(void)
{
	// first get - '<!ELEMENT' S QName 
	if (!_fsm->stream()->compare(L"<!ELEMENT", 9))
	{
		_fsm->stream()->skipChars(9);
		if (!_fsm->stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace in element declaration.");
			_fsm->error(e);
			return false;
		}
		else
			_fsm->stream()->skipBlanks();
		// do we need to make sure the name isn't a peRef?
		std::wstring qname = _fsm->stream()->qName();

		// create tree structure
		ElemDecl decl;
		decl.elementName(qname);
		Doctype * dtd = dynamic_cast<Doctype*>(_fsm->document()->doctype());
		dtd->elementDecl()[qname] = decl;
		_elem = & dtd->elementDecl()[qname];

		// second get - S contentspec

		// eat required whitespace
		if (!_fsm->stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace in element declaration.");
			_fsm->error(e);
			return false;
		}
		else
			_fsm->stream()->skipBlanks();

		// make sure this isn't a peRef?

		if (!_fsm->callState("contentspec"))
		{
			// error - expected contentspec ('EMPTY' | 'ANY' | '(')
			Exception e(Exception::UnknownScanError);
			e.message("expected contentspec in elementDecl");
			_fsm->error(e);
			return false;
		}

		// final step get -  S? '>'
		_fsm->stream()->skipBlanks();
		if (_fsm->stream()->current() == '>')
		{
			_fsm->stream()->nextChar();
		}
		return true;
	}

	return false;
}

// [46]	contentspec		::=	'EMPTY' | 'ANY' | Mixed | children
bool ElementDecl::contentspec(void)
{
	// contentspec starts with 'EMPTY' | 'ANY' | '('
	if (!_fsm->stream()->compare(L"EMPTY", 5))
	{
		_fsm->stream()->skipChars(5);
		_elem->contentspec(ElemDecl::Empty);
		return true;
	}
	if (!_fsm->stream()->compare(L"ANY", 3))
	{
		_fsm->stream()->skipChars(3);
		_elem->contentspec(ElemDecl::Any);
		return true;
	}
	_fsm->stream()->push();

	if (_fsm->callState("Mixed"))
		return true;

	_fsm->stream()->pop();

	if (_fsm->callState("children"))
		return true;

	// should error here - no content model was defined for the elementdecl

	return false;
}

// [51] Mixed			::= '(' S? '#PCDATA' (S? '|' S? Name)* S? ')*' 
//						  | '(' S? '#PCDATA' S? ')'
bool ElementDecl::Mixed(void)
{
	if (_fsm->stream()->current() == '(')
	{
		_fsm->stream()->nextChar();
		_fsm->stream()->skipBlanks();
		if (!_fsm->stream()->compare(L"#PCDATA", 7))
		{
			_fsm->stream()->skipChars(7);
			_fsm->stream()->skipBlanks();

			while (_fsm->stream()->current() == '|')
			{
				_fsm->stream()->nextChar();
				_fsm->stream()->skipBlanks();
				std::wstring qname = _fsm->stream()->qName();
				_fsm->stream()->skipBlanks();
				//!!!FIXME: set _elem mixed name		
			}

			if (_fsm->stream()->current() == ')')
			{
				_fsm->stream()->nextChar();
				if (_fsm->stream()->current() == '*')
				{
						// technically '*' is allowed here by the production even though it might not make sense
						/*
						xmlException e(xmlException::IllegalChar);
						e._message = '*';
						error(e);
						*/
						_fsm->stream()->nextChar();
				}
			}
			return true;
		}
	}
	return false;
}

// [47] children		::= (choice | seq) ('?' | '*' | '+')? 
// [48] cp				::= (Name | choice | seq) ('?' | '*' | '+')? 
// [49] choice			::= '(' S? cp ( S? '|' S? cp )+ S? ')'
// [50] seq				::= '(' S? cp ( S? ',' S? cp )* S? ')'
bool ElementDecl::children(void)
{
	/*
	cp ::= (Name | 
			'(' S? cp ( S? '|' S? cp )+ S? ')' | 
			'(' S? cp ( S? ',' S? cp )* S? ')') 
			('?' | '*' | '+')? 
	cp = content particle
	children = ('(' S? cp ( S? '|' S? cp )+ S? ')' | 
				'(' S? cp ( S? ',' S? cp )* S? ')') 
				('?' | '*' | '+')? 
	? - zero or one - opt
	* - zero or more - mult
	+ - one or more - plus
	tokens:
	 '('			')?' | ')*' | ')+'
	ChildrenGroup (ChildrenGroup2 | Name) (GroupCloseOpt | GroupCloseMult | GroupClosePlus | GroupClose)
	ChildrenGroup = '('
	GroupClose = ')'
	GroupCloseOpt = ')?'
	GroupCloseMult = ')*'
	GroupClosePlus = ')+'
	ChildrenGroup2 = Name | ChildrenChoice | ChildrenSequence
	ChildrenChoice = '|'
	ChildrenSequence = ','
	*/
	if (_fsm->stream()->current() == '(')
	{
		_fsm->stream()->nextChar();
		_fsm->stream()->skipBlanks();
//		token.type(ChildrenGroup);
		return true;
	}
	if (_fsm->stream()->current() == ')')
	{
		_fsm->stream()->nextChar();
/*
		if (_fsm->stream()->current() == '*')
			token.type(GroupCloseMult);
		else if (_fsm->stream()->current() == '+')
			token.type(GroupClosePlus);
		else if (_fsm->stream()->current() == '?')
			token.type(GroupCloseOpt);
		else
			token.type(GroupClose);
		if (token.type() != GroupClose)
			_fsm->stream()->nextChar();
*/
		_fsm->stream()->skipBlanks();
		return true;
	}
	if (_fsm->stream()->current() == '|' || _fsm->stream()->current() == ',')
	{
/*
		if (_fsm->stream()->current() == '|')
			token.type(ChildrenChoice);
		else
			token.type(ChildrenSequence);
*/
		_fsm->stream()->nextChar();
		_fsm->stream()->skipBlanks();
		return true;
	}
	std::wstring qname = _fsm->stream()->qName();
	if (!qname.empty())
	{
/*
		if (_fsm->stream()->current() == '?')
			token.type(ChildrenNameOpt);
		else if (_fsm->stream()->current() == '+')
			token.type(ChildrenNamePlus);
		else if (_fsm->stream()->current() == '*')
			token.type(ChildrenNameMult);
		else
			token.type(ChildrenName);
		if (token.type() != ChildrenName)
			_fsm->stream()->nextChar();
		token.value(qname);
*/
		_fsm->stream()->skipBlanks();
		return true;
	}
	return false;
}
